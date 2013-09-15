#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "tesseract/baseapi.h"

#include "trie/trie.h"
#include "trie/trienode.h"
#include "graph/graph.h"

#include <iostream>
#include <fstream>
#include <utility>
#include <stack>
#include <set>
#include <math.h>
#include <string.h>

static const char* window_name = "haha";
static const int columns = 4;
static const int rows = 4;

static const std::vector<std::pair<int, int>> adjacent = 
{{-1, -1}, {-1, 0}, {-1, 1},
  {0, -1},/*{0, 0},*/{0, 1},
  {1, -1},  {1, 0},  {1, 1}};

std::vector<std::vector<cv::Point2f>> findSquares(cv::Mat& image)
{
  std::vector<std::vector<cv::Point2f>> squares;

  cv::Mat pyr, timg;
  // down-scale and upscale the image to filter out the noise
  cv::pyrDown(image, pyr, cv::Size(image.cols/2, image.rows/2));
  cv::pyrUp(pyr, timg, image.size());

  // use only one channel from an image
  int from_to[] = {0, 0};
  cv::Mat single_channel(image.size(), CV_8U);
  cv::mixChannels(&timg, 1, &single_channel, 1, from_to, 1);

  // detect edges using canny algorithm
  cv::Mat bw;
  cv::Canny(single_channel, bw, 100, 200, 3);
   
  // dilate the image in order to remove possible gaps on edges
  cv::dilate(bw, bw, cv::Mat(), cv::Point2f(-1, -1));

  // extract contours from the image
  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(bw, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

  std::vector<cv::Point> approx;
  for (auto contour : contours)
  {
    // approximate contour with accuracy proportional
    // to the contour perimeter
    cv::approxPolyDP(cv::Mat(contour), approx,
        cv::arcLength(cv::Mat(contour), true) * 0.03, true);

    // squares we are looking for have four vertices
    // have area bigger than 0.005 part of total image area
    // and are convex
    if (approx.size() == 4 && std::fabs(cv::contourArea(contour)) > image.size().area() * 0.0001 &&
        cv::isContourConvex(approx))
    {
      // also, bounding rectangle's dimensions must be square-like
      cv::Rect bounding_rect = cv::boundingRect(cv::Mat(approx));
      if ((float)bounding_rect.height < (float)bounding_rect.width * 1.2 &&
          (float)bounding_rect.height > (float)bounding_rect.width * 0.8)
      {
        std::vector<cv::Point2f> result;
        cv::Mat(approx).copyTo(result);
        squares.push_back(result);
      }
    }
  }
  return squares;
}

// function was taken from http://opencv-code.com/tutorials/automatic-perspective-correction-for-quadrilateral-objects/
void sortCorners(std::vector<cv::Point2f>& corners, cv::Point centre)
{
  std::vector<cv::Point2f> top, bottom;

  for (auto corner : corners)
    if (corner.y < centre.y) top.push_back(corner);
    else bottom.push_back(corner);

  cv::Point2f top_left = top[0].x > top[1].x ? top[1] : top[0];
  cv::Point2f top_right = top[0].x > top[1].x ? top[0] : top[1];
  cv::Point2f bottom_left = bottom[0].x > bottom[1].x ? bottom[1] : bottom[0];
  cv::Point2f bottom_right = bottom[0].x > bottom[1].x ? bottom[0] : bottom[1];

  corners.clear();
  corners.push_back(top_left);
  corners.push_back(top_right);
  corners.push_back(bottom_right);
  corners.push_back(bottom_left);
}

std::vector<std::vector<cv::Mat>> cutSquares(cv::Mat image, std::vector<std::vector<cv::Point2f>> squares)
{
  if (squares.size() != 16) return std::vector<std::vector<cv::Mat>>();
  // find mass centres and sort square's corners
  std::vector<cv::Point2f> centres;
  std::vector<std::pair<std::pair<int, int>, int>> sorted;
  for (int i = 0; i < squares.size(); ++i)
  {
    cv::Point2f centre(0, 0);
    for (auto p : squares[i]) centre += p;

    centre *= 1.0 / squares[i].size();
    sortCorners(squares[i], centre);
    centres.push_back(centre);
    sorted.push_back(std::make_pair(std::make_pair(centre.x, centre.y), i));
  }

  // sort squares
  sort(sorted.begin(), sorted.end());

  std::vector<std::vector<cv::Mat>> table(rows, std::vector<cv::Mat>(columns));
  for (int i = 0; i < rows; ++i)
    for (int j = 0; j < columns; ++j)
    {
      int index = sorted[i * columns + j].second;
      int square_size = std::min(image.size().height, image.size().width) / 4;
      cv::Mat result = cv::Mat(square_size, square_size, CV_8UC3);

      std::vector<cv::Point2f> result_points;
      result_points.push_back(cv::Point2f(0, 0));
      result_points.push_back(cv::Point2f(result.cols, 0));
      result_points.push_back(cv::Point2f(result.cols, result.rows));
      result_points.push_back(cv::Point2f(0, result.rows));

      cv::Mat transformation_matrix =
        cv::getPerspectiveTransform(squares[index], result_points);

      cv::warpPerspective(image, result, transformation_matrix, result.size());
      table[i][j] = result;
    }
  return table;
}

std::string extractLetters(cv::Mat square, bool check_dbg = false)
{
  cv::Mat pyr, timg;
  // down-scale and upscale the image to filter out the noise
  cv::pyrDown(square, pyr, cv::Size(square.cols/2, square.rows/2));
  cv::pyrUp(pyr, timg, square.size());
  timg = timg(cv::Rect(timg.size().width * 0.05, timg.size().height * 0.05, 
        timg.size().width * 0.9, timg.size().height * 0.9));

  int from_to[] = {2, 0};
  cv::Mat single_channel(timg.size(), CV_8U);
  cv::mixChannels(&timg, 1, &single_channel, 1, from_to, 1);
  cv::threshold(single_channel, single_channel, 140, 255, cv::THRESH_BINARY_INV);
  cv::Mat bw;
  cv::Canny(single_channel, bw, 0, 50, 3);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(bw, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
  std::vector<cv::Point> contour;
  for (auto cont : contours)
  {
    cv::Rect bounding_rect = cv::boundingRect(cv::Mat(cont));
    if (bounding_rect.height < square.size().height * 0.9 &&
        bounding_rect.height > square.size().height * 0.3)
      for (auto point : cont)
        contour.push_back(point);
  }
  if (contour.empty()) return "";
  cv::Rect bounds = cv::boundingRect(cv::Mat(contour));
  cv::Rect new_bounds;
  new_bounds.x = 0;
  new_bounds.width = timg.size().width;
  new_bounds.y = std::max(bounds.y - bounds.height * 0.1, 0.0);
  new_bounds.height = std::min(bounds.height + bounds.height * 0.2,
      (double)timg.size().height - new_bounds.y);
  cv::Mat letters = timg(new_bounds).clone();
  cv::Mat letters_single_channel(letters.size(), CV_8U);
  cv::mixChannels(&letters, 1, &letters_single_channel, 1, from_to, 1);
  cv::threshold(letters_single_channel, letters_single_channel,
      180, 255, cv::THRESH_BINARY_INV);
  cv::rectangle(letters_single_channel, cv::Point(0,0),
      cv::Point(bounds.x, timg.size().height), cv::Scalar(255,255,255), CV_FILLED);
  cv::rectangle(letters_single_channel, cv::Point(bounds.x+bounds.width,0),
      cv::Point(timg.size().width, timg.size().height), cv::Scalar(255,255,255), CV_FILLED);

  tesseract::TessBaseAPI tess;
  tess.Init(NULL, "eng");
  tess.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ/-");
  tess.SetImage((uchar*)letters_single_channel.data,
      letters_single_channel.cols, letters_single_channel.rows, 1,
      letters_single_channel.cols);
  std::string str;
  char* out = tess.GetUTF8Text();
  for (int i = 0; out[i] != '\0'; ++i)
    if (isalpha(out[i]) || out[i] == '/')
      str.push_back(out[i]);
  if (str == "" && bounds.width < bounds.height * 0.3)
      str = "I";
  //cv::imshow(window_name, letters_single_channel);
  //std::cout << str << std::endl;
  //cv::waitKey();
  return str;
}

void dfs(Graph& graph, int index, Trie& trie, std::string str, std::vector<bool>& visited, std::set<std::string>& results)
{
  if (trie.contains(str) && str.length() >= 3) results.insert(str);
  for (auto ind : graph.vertices[index].edges)
  {
    if (!visited[ind] &&
        trie.getNode(str + graph.vertices[ind].content) != NULL)
    {
      visited[ind] = true;
      dfs(graph, ind, trie, str + graph.vertices[ind].content, visited, results);
      visited[ind] = false;
    }
  }
}

void getWords(std::vector<std::vector<std::string>>& table, Trie& trie)
{
  Graph graph(table);
  std::set<std::string> results;
  for (int i = 0; i < graph.vertices.size(); ++i)
  {
    std::string str = graph.vertices[i].content;
    std::vector<bool> visited(graph.vertices.size(), false);
    visited[i] = true;
    dfs(graph, i, trie, str, visited, results);
  }
  std::vector<std::string> sorted(results.begin(), results.end());
  std::sort(sorted.begin(), sorted.end(), [](std::string a, std::string b)
      { return (a.length() > b.length()); });
  for (auto str : sorted)
    std::clog << str << "  ";
}

int main()
{
  // Filling the wordlist
  Trie trie;
  std::ifstream wordlist("./wordlist/CROSSWD.TXT");
  std::string line;
  while (std::getline(wordlist, line)) trie.add(line);

  // Launching camera
  cv::VideoCapture cap(0);
  if (!cap.isOpened())
  {
    std::clog << "Could not open webcam" << std::endl;
    return -1;
  }
  // Extracting letters
  cv::Mat frame = cv::imread("haha.png", 1);
  std::vector<std::vector<std::string>> table(rows,
      std::vector<std::string>(columns));
  while(true)
  {
    if (cv::waitKey(30) >= 0)
    {
      cv::imwrite("haha.png", frame);
      break;
    }
    cap >> frame;
    cv::namedWindow(window_name, 1);
    std::vector<std::vector<cv::Point2f>> squares = findSquares(frame);
    std::vector<std::vector<cv::Mat>> images_table = cutSquares(frame, squares);
    if (images_table.empty())
    {
      cv::imshow(window_name, frame);
      continue;
    }

    for (int i = 0; i < images_table.size(); ++i)
      for (int j = 0; j < images_table[i].size(); ++j)
      {
        if (i == 3 && j == 0) table[i][j] = extractLetters(images_table[i][j], true);
        table[i][j] = extractLetters(images_table[i][j]);
        cv::putText(frame, table[i][j], cv::Point(50*(i+1), 50*(j+1)), cv::FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0));
      }
    cv::imshow(window_name, frame);
    //cv::waitKey();
  }
  // Processing the table
  getWords(table, trie);
  return 0;
}
