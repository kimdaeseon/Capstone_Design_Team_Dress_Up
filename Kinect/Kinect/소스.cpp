#include <k4a/k4a.h>
#include <k4abt.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include "KinectClient.h"

struct color_point_t {
    float xyz[3];
    int number;
    int body;
    int xth;
    int yth;
    int color[3];
};

int main(int argc, char** argv) 
{
    int returnCode = 1;
    k4a_device_t device = NULL;
    const int32_t TIMEOUT_IN_MS = 10000;
    k4a_transformation_t transformation = NULL;
    k4a_capture_t capture = NULL;
    std::string file_name = "";
    uint32_t device_count = 0;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    uint8_t deviceId = K4A_DEVICE_DEFAULT;

    SocketClient client;

    //키넥트 디바이스 개수
    device_count = k4a_device_get_installed_count();

    //개수가 확인되지 않으면 종료
    if (device_count == 0) {
        printf("Please check the Kinect camera connection and try again..\n");
        return 0;
    }
    else {
        printf("The Kinect camera connection has been confirmed.\n");
    }

    //디바이스 오픈
    if (K4A_RESULT_SUCCEEDED != k4a_device_open(deviceId, &device)) {
        printf("Failed to open device\n");
        return 0;
    }

    //키넥트 카메라 모드 설정
    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.synchronized_images_only = true;

    // ensures that depth and color images are both available in the capture
    k4a_calibration_t calibration;

    //Get calibration
    if (K4A_RESULT_SUCCEEDED != k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration)) {
        printf("Failed to get calibration\n");
        return 0;
    }

    printf("When you are ready, press the 'Y' button.\n");
    printf("If you want to end the program, press the 'Q' button.\n");

    char input;
    while (cin>>input) {
        if (input == 'y' || input == 'Y') {
            break;
        }
        else if(input =='Q' || input =='q') {
            k4a_device_close(device);
            printf("The Program Ends.\n");
            return 0;
        }
        else {
            printf("Please re-enter\n");
        }
    }

    transformation = k4a_transformation_create(&calibration);

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config)) {
        printf("Failed to start cameras\n");
        return 0;
    }

    // Get a capture
    switch (k4a_device_get_capture(device, &capture, TIMEOUT_IN_MS)) {
    case K4A_WAIT_RESULT_SUCCEEDED:
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        printf("Timed out waiting for a capture\n");
        k4a_device_close(device);
        return 0;
    case K4A_WAIT_RESULT_FAILED:
        printf("Failed to read a capture\n");
        k4a_device_close(device);
        return 0;
    }

    // Get a color image
    color_image = k4a_capture_get_color_image(capture);
    if (color_image == 0) {
        printf("Failed to get color image from capture\n");
        k4a_device_close(device);
        return 0;
    }

    // Get a depth image
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0) {
        printf("Failed to get depth image from capture\n");
        k4a_device_close(device);
        return 0;
    }

    // custom start - image create
    int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
    int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);

    k4a_image_t point_cloud_image = NULL;
    k4a_image_t body_index_map = NULL;

    //create body tracker
    k4abt_tracker_t body_tracker;
    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
    if (K4A_RESULT_SUCCEEDED != k4abt_tracker_create(&calibration, tracker_config, &body_tracker)) {
        printf("Failed to create body tracker\n");
        k4a_device_close(device);
        return 0;
    }

    //extract body frame
    k4abt_frame_t body_frame = NULL;
    k4abt_tracker_enqueue_capture(body_tracker, capture, TIMEOUT_IN_MS);
    k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(body_tracker, &body_frame, TIMEOUT_IN_MS);
    if (K4A_RESULT_SUCCEEDED != pop_frame_result) {
        printf("Failed to pop body frame!\n");
        k4a_device_close(device);
        return 0;
    }

    //create body index map image
    body_index_map = k4abt_frame_get_body_index_map(body_frame);
    uint8_t* body_index_map_data = (uint8_t*)(void*)k4a_image_get_buffer(body_index_map);

    //create point cloud image
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM, depth_image_width_pixels,
        depth_image_height_pixels, 3 * depth_image_width_pixels * (int)sizeof(int16_t), &point_cloud_image)) {
        printf("Failed to create point cloud image\n");
        k4a_device_close(device);
        return false;
    }

    //next step - point cloud generate
    if (K4A_RESULT_SUCCEEDED != k4a_transformation_depth_image_to_point_cloud(transformation, depth_image, K4A_CALIBRATION_TYPE_DEPTH, point_cloud_image)) {
        printf("Failed to compute point cloud\n");
        k4a_device_close(device);
        return false;
    }

    // 3rd step: save point cloud
    std::vector<color_point_t> points;

    int width = k4a_image_get_width_pixels(point_cloud_image);
    int height = k4a_image_get_height_pixels(point_cloud_image);
    int16_t* point_cloud_image_data = (int16_t*)(void*)k4a_image_get_buffer(point_cloud_image);

    int number = 1;
    int left = 1000, right = 0;
    int top = 1000, bottom = 0;

    for (int i = 0; i < width * height; i++) {
        if (body_index_map_data[i] == K4ABT_BODY_INDEX_MAP_BACKGROUND) {
            continue;
        }
        if (left > (i % width)) {
            left = (i % width);
        }
        if (right < (i % width)) {
            right = (i % width);
        }
        if (top > (i / width)) {
            top = (i / width);
        }
        if (bottom < (i / width)) {
            bottom = (i / width);
        }
    }

    //depth camera 시점에서 color image 획득
    k4a_image_t transformed_color_image = NULL;
    if (K4A_RESULT_SUCCEEDED != k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, depth_image_width_pixels, depth_image_height_pixels,
        depth_image_width_pixels * 4 * (int)sizeof(uint8_t), &transformed_color_image)) {
        printf("Failed to create transformed color image\n");
        k4a_device_close(device);
        return false;
    }
    if (K4A_RESULT_SUCCEEDED != k4a_transformation_color_image_to_depth_camera(transformation, depth_image, color_image, transformed_color_image)) {
        printf("Failed to transform color_image to depth camera\n");
        k4a_device_close(device);
        return false;
    }
    uint8_t* transformed_color_image_data = (uint8_t*)(void*)k4a_image_get_buffer(transformed_color_image);

    for (int i = 0; i < width * height; i++) {
        color_point_t point;
        point.xyz[0] = point_cloud_image_data[3 * i];
        point.xyz[1] = point_cloud_image_data[3 * i + 1];
        point.xyz[2] = point_cloud_image_data[3 * i + 2];
        point.body = body_index_map_data[i];
        point.xth = i % width;
        point.yth = i / width;
        point.color[0] = transformed_color_image_data[4 * i];
        point.color[1] = transformed_color_image_data[4 * i + 1];
        point.color[2] = transformed_color_image_data[4 * i + 2];

        if (point.xyz[2] == 0 || point.body == K4ABT_BODY_INDEX_MAP_BACKGROUND) {
            point.number = 0;
        }
        else {
            point.number = number;
            number++;
        }
        points.push_back(point);
    }

    //triangulation
    std::vector<int> triangle;
    std::vector<std::pair<float, float>> texture;

    for (int i = 0; i < width * (height - 1) - 1; i++) {
        //i번째 z좌표, (i+1)번째 z좌표, (i+width)번째 z좌표가 모두 0이 아닐 때 triangle생성
        if ((point_cloud_image_data[3 * i + 2] != 0 && point_cloud_image_data[3 * i + 5] != 0 && point_cloud_image_data[3 * (i + width) + 2] != 0) &&
            (body_index_map_data[i] != K4ABT_BODY_INDEX_MAP_BACKGROUND && body_index_map_data[i + 1] != K4ABT_BODY_INDEX_MAP_BACKGROUND && body_index_map_data[i + width] != K4ABT_BODY_INDEX_MAP_BACKGROUND)) {
            //반시계방향
            triangle.push_back(points[i + 1].number);
            triangle.push_back(points[i].number);
            triangle.push_back(points[i + width].number);
        }

        //i+1번째 z좌표, (i+width)번째 z좌표, (i+width+1)번째 z좌표가 모두 0이 아닐 때 triangle 생성
        if ((point_cloud_image_data[3 * i + 5] != 0 && point_cloud_image_data[3 * (i + width) + 2] != 0 && point_cloud_image_data[3 * (i + width + 1) + 2] != 0) &&
            (body_index_map_data[i + 1] != K4ABT_BODY_INDEX_MAP_BACKGROUND && body_index_map_data[i + width] != K4ABT_BODY_INDEX_MAP_BACKGROUND && body_index_map_data[i + width + 1] != K4ABT_BODY_INDEX_MAP_BACKGROUND)) {
            //반시계방향
            triangle.push_back(points[i + 1].number);
            triangle.push_back(points[i + width].number);
            triangle.push_back(points[i + width + 1].number);
        }
    }

    //points vector에서 z좌표가 0인 점 제거, body로 인식되지 않은 점 제거
    points.erase(std::remove_if(points.begin(), points.end(), [](color_point_t x)->bool {return x.number == 0; }),
        points.end());

    //Joint 좌표획득
    k4abt_skeleton_t skeleton;
    size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);
    if (num_bodies == 0) {
        printf("Can't recognize body!\n");
        k4a_device_close(device);
        return 0;
    }
    printf("%zu bodies are detected!\n", num_bodies);

    for (size_t i = 0; i < num_bodies; i++) {
        k4abt_frame_get_body_skeleton(body_frame, i, &skeleton);
        uint32_t id = k4abt_frame_get_body_id(body_frame, i);
    }

    k4a_image_release(point_cloud_image);
    k4a_image_release(depth_image);
    k4a_image_release(color_image);
    k4a_image_release(transformed_color_image);

    k4abt_frame_release(body_frame);
    k4a_image_release(body_index_map);

    std::cout << points.size() << std::endl;

    client.connection();

    //vertex 좌표 정보
    for (size_t i = 0; i < points.size(); ++i) {
        string s;
        s = "v ";
        s += to_string(points[i].xyz[0]);
        s += " ";
        s += to_string(points[i].xyz[1]);
        s += " ";
        s += to_string(points[i].xyz[2]);
        s += "\n";

        
        client.sendData(s);
        client.recvFlag();
    }
    //texture 정보
    for (size_t i = 0; i < points.size(); i++) {
        string s;
        s = "vt ";
        s += to_string((float)(points[i].xth - left) / (right - left));
        s += " ";
        s+= to_string((float)(points[i].yth - top) / (bottom - top));
        s += "\n";
 
        client.sendData(s);
        client.recvFlag();
    }

    //color 정보
    for (size_t i = 0; i < points.size(); i++) {
        string s;
        s = "c ";
        s += to_string(points[i].color[2]);
        s += " ";
        s += to_string(points[i].color[1]);
        s += " ";
        s += to_string(points[i].color[0]);
        s += "\n";

        client.sendData(s);
        client.recvFlag();
    }

    //face 정보
    for (size_t i = 0; i < triangle.size() / 3; i++) {
        string s;
        s = "f ";
        s += to_string(triangle[3 * i]);
        s += "/";
        s += to_string(triangle[3 * i]);
        s += "/";
        s += to_string(triangle[3 * i]);
        s += " ";
        s += to_string(triangle[3 * i + 1]);
        s += "/";
        s += to_string(triangle[3 * i + 1]);
        s += "/";
        s += to_string(triangle[3 * i + 1]);
        s += " ";
        s += to_string(triangle[3 * i + 2]);
        s += "/";
        s += to_string(triangle[3 * i + 2]);
        s += "/";
        s += to_string(triangle[3 * i + 2]);
        s += "\n";
      

        client.sendData(s);
        client.recvFlag();
    }

    string s[11];

    //Joint 좌표 정보(가운데 골반)
    s[0] = "b ";
    s[0] += to_string((float)skeleton.joints[0].position.xyz.x);
    s[0] += " ";
    s[0] += to_string((float)skeleton.joints[0].position.xyz.y);
    s[0] += " ";
    s[0] += to_string((float)skeleton.joints[0].position.xyz.z);
    s[0] += "\n";

    client.sendData(s[0]);
    client.recvFlag();


    //왼쪽 엉덩이
    s[1] = "b ";
    s[1] += to_string((float)skeleton.joints[18].position.xyz.x);
    s[1] += " ";
    s[1] += to_string((float)skeleton.joints[18].position.xyz.y);
    s[1] += " ";
    s[1] += to_string((float)skeleton.joints[18].position.xyz.z);
    s[1] += "\n";
 
    client.sendData(s[1]);
    client.recvFlag();


    //오른쪽 엉덩이
    s[2] = "b ";
    s[2] += to_string((float)skeleton.joints[22].position.xyz.x);
    s[2] += " ";
    s[2] += to_string((float)skeleton.joints[22].position.xyz.y);
    s[2] += " ";
    s[2] += to_string((float)skeleton.joints[22].position.xyz.z);
    s[2] += "\n";

    client.sendData(s[2]);
    client.recvFlag();


    //왼쪽 발목
    s[3] = "b ";
    s[3] += to_string((float)skeleton.joints[20].position.xyz.x);
    s[3] += " ";
    s[3] += to_string((float)skeleton.joints[20].position.xyz.y);
    s[3] += " ";
    s[3] += to_string((float)skeleton.joints[20].position.xyz.z);
    s[3] += "\n";
 
    client.sendData(s[3]);
    client.recvFlag();
    
    //오른쪽 발목
    s[4] = "b ";
    s[4] += to_string((float)skeleton.joints[24].position.xyz.x);
    s[4] += " ";
    s[4] += to_string((float)skeleton.joints[24].position.xyz.y);
    s[4] += " ";
    s[4] += to_string((float)skeleton.joints[24].position.xyz.z);
    s[4] += "\n";

    client.sendData(s[4]);
    client.recvFlag();


    //가운데 골반
    s[5] = "b ";
    s[5] += to_string((float)skeleton.joints[0].position.xyz.x);
    s[5] += " ";
    s[5] += to_string((float)skeleton.joints[0].position.xyz.y);
    s[5] += " ";
    s[5] += to_string((float)skeleton.joints[0].position.xyz.z);
    s[5] += "\n";

    client.sendData(s[5]);
    client.recvFlag();
    
    //목
    s[6] = "b ";
    s[6] += to_string((float)skeleton.joints[3].position.xyz.x);
    s[6] += " ";
    s[6] += to_string((float)skeleton.joints[3].position.xyz.y);
    s[6] += " ";
    s[6] += to_string((float)skeleton.joints[3].position.xyz.z);
    s[6] += "\n";

    client.sendData(s[6]);
    client.recvFlag();
   
    //왼쪽 손목
    s[7] = "b ";
    s[7] += to_string((float)skeleton.joints[7].position.xyz.x);
    s[7] += " ";
    s[7] += to_string((float)skeleton.joints[7].position.xyz.y);
    s[7] += " ";
    s[7] += to_string((float)skeleton.joints[7].position.xyz.z);
    s[7] += "\n";

    client.sendData(s[7]);
    client.recvFlag();
    
    //오른쪽 손목
    s[8] = "b ";
    s[8] += to_string((float)skeleton.joints[14].position.xyz.x);
    s[8] += " ";
    s[8] += to_string((float)skeleton.joints[14].position.xyz.y);
    s[8] += " ";
    s[8] += to_string((float)skeleton.joints[14].position.xyz.z);
    s[8] += "\n";
  
    client.sendData(s[8]);
    client.recvFlag();
    
    //CLAVICLE_LEFT
    s[9] = "b ";
    s[9] += to_string((float)skeleton.joints[4].position.xyz.x);
    s[9] += " ";
    s[9] += to_string((float)skeleton.joints[4].position.xyz.y);
    s[9] += " ";
    s[9] += to_string((float)skeleton.joints[4].position.xyz.z);
    s[9] += "\n";

    client.sendData(s[9]);
    client.recvFlag();
    
    //CLAVICLE_RIGHT
    s[10] = "b ";
    s[10] += to_string((float)skeleton.joints[11].position.xyz.x);
    s[10] += " ";
    s[10] += to_string((float)skeleton.joints[11].position.xyz.y);
    s[10] += " ";
    s[10] += to_string((float)skeleton.joints[11].position.xyz.z);
    s[10] += "\n";

    client.sendData(s[10]);
    client.recvFlag();

    k4a_device_close(device);

    client.close();
   
    return 0;
}
