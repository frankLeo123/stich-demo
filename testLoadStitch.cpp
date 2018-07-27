#include <vector>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <gflags/gflags.h>
#include <fstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <time.h>
using namespace std;
using namespace cv;

//路径和格式
DEFINE_string(config_dir, "../shangqi/", "load config dir");
//DEFINE_string(config_dir, "../config/shangqi/", "load config dir");
DEFINE_string(format, ".jpg", "load image file format");

int ReadFlowFile(const std::string& filePath, cv::Mat& flow){

	auto dot = filePath.find_last_of('.');
	std::string dotSubStr = filePath.substr(dot + 1, filePath.size() - dot);
	if (dotSubStr != "flo"){
		return 2;
	}

	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs.is_open()){
		return 3;
	}
	char str[4];
	ifs.read(str, 4);
	int width, height;
	ifs.read((char*)&width, 4);
	ifs.read((char*)&height, 4);

	flow = cv::Mat::zeros(height, width, CV_32FC2);
	for (int r = 0; r < flow.rows; r++){
		for (int c = 0; c < flow.cols; c++){
			float u, v;
			ifs.read((char*)&u, 4);
			ifs.read((char*)&v, 4);
			flow.at<cv::Point2f>(r, c) = cv::Point2f(u, v);
		}
	}
	ifs.close();
	return 0;
}

int ReadConfig(string filename, vector<Point>& vec_st_points, Size& final_size)
{
    cv::FileStorage ifile(filename, cv::FileStorage::READ);
    if(!ifile.isOpened()) {
        cout<< "failed to read stitch file: " <<filename<<endl;
        return -1;
    }

    vec_st_points.resize(4);
    ifile["output_size"]>>final_size;
    for(int i=0;i<4; i++)
    {
        ifile["start_point_"+to_string(i)]>>vec_st_points[i];
    }
    ifile.release();
    return 0;
}

//读取文件名
std::vector<std::string> ListDir2(string src_dir) {
 std::vector<std::string> files;
 #ifdef WIN32
  _finddata_t file;
  long lf;
  if ((lf = _findfirst(src_dir.c_str(), &file)) == -1) {
    cout << src_dir << " not found!!!" << endl;
  } else {
    while (_findnext(lf, &file) == 0) {
      if (strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0) continue;
      auto res = ProcessFileName(file.name);
      files[res.first] = res.second;
    }
  }
  _findclose(lf);
 #endif

 #ifdef __linux__
  DIR *dir;
  struct dirent *ptr;

  if ((dir = opendir(src_dir.c_str())) == NULL) {
    // LOG(ERROR) << "Open dir error...: " << src_dir;
    exit(1);
  }
  while ((ptr = readdir(dir)) != NULL) {
    if (strcmp(ptr->d_name, ".") == 0 ||
        strcmp(ptr->d_name, "..") == 0) {  // cur  or parent
      continue;
    } else if (ptr->d_type == 8) {  // file
      //auto res = ProcessFileName(ptr->d_name);
      //files[res.first] = res.second;
      files.push_back(ptr->d_name);
    } else if (ptr->d_type == 10) {  // link file
      continue;
    } else if (ptr->d_type == 4) {  // dir
      // files.push_back(ptr->d_name);
      continue;
    }
  }
  closedir(dir);
 #endif
  sort(files.begin(), files.end());
  return files;
}

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc,&argv, true);
    // string tags[] = {"front", "right", "back", "left"};
    // std::string camera_to_pos[4] = {"camera00", "camera01", "camera02", "camera03"};       
    std::string camera_to_pos[4] = {"front", "right", "back", "left"};   
    vector<Mat> vec_src(4), vec_sep_map(4), vec_sep_mask(4);
    vector<Point> vec_st_points;
    //时间戳形式读取pics
    #if 1
    std::string image_file_path = FLAGS_config_dir +"image/";
    std::vector<string> image_names[4];
    int min_image_num = INT_MAX;

    std::stringstream ss;

    std::vector<long> timestamps[4];
    std::vector<long> timestamp_;

    for(int i = 0; i < 4; ++i)
    {
        image_names[i] = ListDir2(image_file_path + camera_to_pos[i]);
        min_image_num = std::min(min_image_num, (int)image_names[i].size());
        timestamps[i].resize(image_names[i].size());
        
        for(size_t index = 0; index < image_names[i].size(); ++index)
        {
            timestamps[i][index] = std::stoll(image_names[i][index].substr(0, image_names[i][index].length()-4));
            
        }
        std::cout<<image_names[i][0]<<endl;
        sort(timestamps[i].begin(), timestamps[i].end());
    }  
    #endif
    int num = 0;
    int num_error = 0;

    #if 1
    for(size_t index = 0; index < image_names[0].size(); ++index)
    {
        Size final_size;
        for(int i=0; i<4; i++)
        {
            vec_src[i] = imread(image_file_path  + camera_to_pos[i] + "/"  + std::to_string(timestamps[i][index])+ FLAGS_format); //+ "_"+ std::to_string(index) + "_"+ std::to_string(i));            
            // vec_src[i] = imread(image_file_path  + camera_to_pos[i] + "/"  + image_names[i][index]); //+ "_"+ std::to_string(index) + "_"+ std::to_string(i));
            // vec_src[i] = imread(FLAGS_config_dir + "image/" + tags[i] + "/"  + tags[i] + FLAGS_format);
            std::cout<<vec_src[i].size()<<std::endl;
            
            // std::cout<<index<<" :  "<<timestamps[0][index]<<endl;
            // std::cout<<index<<" :  "<<vec_src[i].size()<<endl;
            // if((timestamps[0][index] == timestamps[1][index]) && (timestamps[0][index] == timestamps[2][index]) &&(timestamps[0][index] == timestamps[3][index]))
            //     {}
                // imwrite("test123.png",vec_src[i]);
            // else{
            //     num++;
            //     std::cout<<index<<":  "<<timestamps[0][index]<<endl;
            //     std::cout<<index<<":  "<<timestamps[1][index]<<endl;
            //     std::cout<<index<<":  "<<timestamps[2][index]<<endl;
            //     std::cout<<index<<":  "<<timestamps[3][index]<<endl;
            // }
            

            ReadFlowFile(FLAGS_config_dir+"map_"+to_string(i)+".flo", vec_sep_map[i]);
        };

        ReadConfig(FLAGS_config_dir+"config.yml", vec_st_points, final_size);
        Mat dst = Mat::zeros(final_size, CV_8UC3);

        // cout<<"vec_st_points "<<vec_st_points<<endl;
        // cout<<"output size "<<final_size<<endl;

        for(int i=0; i<4; i++)
        {
            Mat tmp;
            remap(vec_src[i], tmp, vec_sep_map[i], Mat(), CV_INTER_LINEAR);
            Rect rect = Rect(vec_st_points[i]-vec_st_points[3], vec_sep_map[i].size());
            // cout<<"for index "<<to_string(i)<<": "<<rect<<endl;
            tmp.copyTo(dst(rect));
        }
        //输出结果
        // imwrite(FLAGS_config_dir+ "result/" + "load_sttich.jpg", dst);    
        cout<<"num ==  "<<index + 1 <<"  name: "<<std::to_string(timestamps[0][index])<<std::endl;


        imwrite(FLAGS_config_dir+ "result/" +std::to_string(timestamps[0][index]) +".jpg", dst);        
        // imwrite(FLAGS_config_dir+ "result/" +std::to_string(index) +".jpg", dst);
            
    }
    #endif
    //判断掉帧
    #if 1
        ifstream fin("/home/frank/Documents/shangqi/in_hz_10/video_time_0.txt");  

        string s;  

        while( getline(fin,s) )
        {    
            cout << "Read from file: " << s << endl; 
            // string [] b = s.split(" ");
            long temp = std::stoll(s.substr(s.find(' '),14));
            timestamp_.push_back(std::stoll(s.substr(s.find(' '),14)));
            // timestamp_.push_back(std::stoll(s));
            cout<<"time: "<<temp<<endl;
        }
        sort(timestamp_.begin(), timestamp_.end());
        for(size_t index = 0; index < timestamp_.size(); ++index)
        {
            /* code */
            int error_longht = timestamp_[index+1]-timestamp_[index];
            // 10hz = 100 101; 30hz = 33;
            if (error_longht == 99){
                std::cout<<"error! long == "<< error_longht<< "; at :" << index<< std::endl;
                ++num_error;
            }
        }
        std::cout<< "error 错位  " <<num<<std::endl;
        std::cout<< "error 错位" <<(double)num/timestamps[0].size()<<std::endl;
        std::cout<< "error 掉帧  " <<num_error<<std::endl;
        std::cout<< "error 掉帧" <<(double)num_error/timestamps[0].size()<<std::endl;
    #endif    
    
    return 0;
}
