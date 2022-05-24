
#include <k4a/k4a.h>
#include <k4abt.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

struct color_point_t {
    float xyz[3];
    int number;
    int body;
    int xth;
    int yth;
};

int main() {
    int returnCode = 1;
    k4a_device_t device = NULL;
    const int32_t TIMEOUT_IN_MS = 10000;
    k4a_transformation_t transformation = NULL;
    k4a_transformation_t transformation_color_downscaled = NULL;
    k4a_capture_t capture = NULL;
    std::string file_name = "";
    uint32_t device_count = 0;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t color_image_downscaled = NULL;
    uint8_t deviceId = K4A_DEVICE_DEFAULT;

    //키넥트 디바이스 개수
    device_count = k4a_device_get_installed_count();

    //개수가 확인되지 않으면 종료
    if (device_count == 0) {
        printf("No K4A devices found\n");
        return 0;
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
    k4abt_tracker_enqueue_capture(body_tracker,capture,TIMEOUT_IN_MS);
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
        depth_image_height_pixels, 3*depth_image_width_pixels * (int)sizeof(int16_t), &point_cloud_image)) {
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
    int16_t *point_cloud_image_data = (int16_t *)(void *)k4a_image_get_buffer(point_cloud_image);

    int number = 1;
    int left = 1000, right = 0;
    int top = 1000, bottom = 0;

    for (int i = 0; i < width * height; i++) {
        if (body_index_map_data[i] == K4ABT_BODY_INDEX_MAP_BACKGROUND) {
            continue;
        }
        if (left > (i%width)) {
            left = (i % width);
        }
        if (right < (i%width)) {
            right = (i % width);
        }
        if (top > (i/width)) {
            top = (i / width);
        }
        if (bottom < (i / width)) {
            bottom = (i / width);
        }
    }

    for (int i = 0; i < width * height; i++) {
        color_point_t point;
        point.xyz[0] = point_cloud_image_data[3*i];
        point.xyz[1] = point_cloud_image_data[3*i+1];
        point.xyz[2] = point_cloud_image_data[3*i+2];
        point.body = body_index_map_data[i];
        point.xth = i % width;
        point.yth = i / width;

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
    std::vector<std::pair<float,float>> texture;

    for (int i = 0; i < width * (height-1)-1; i++) {
        //i번째 z좌표, (i+1)번째 z좌표, (i+width)번째 z좌표가 모두 0이 아닐 때 triangle생성
        if ((point_cloud_image_data[3*i+2] !=0 && point_cloud_image_data[3*i+5] !=0 && point_cloud_image_data[3*(i+width)+2] !=0) && 
            (body_index_map_data[i]!=K4ABT_BODY_INDEX_MAP_BACKGROUND && body_index_map_data[i+1]!= K4ABT_BODY_INDEX_MAP_BACKGROUND &&body_index_map_data[i+width]!= K4ABT_BODY_INDEX_MAP_BACKGROUND)) {  
            //반시계방향
            triangle.push_back(points[i + 1].number);          
            triangle.push_back(points[i].number);
            triangle.push_back(points[i + width].number);
        }

        //i+1번째 z좌표, (i+width)번째 z좌표, (i+width+1)번째 z좌표가 모두 0이 아닐 때 triangle 생성
        if ((point_cloud_image_data[3*i+5] !=0 && point_cloud_image_data[3*(i+width)+2] != 0 && point_cloud_image_data[3*(i+width+1)+2] !=0) && 
            (body_index_map_data[i+1]!= K4ABT_BODY_INDEX_MAP_BACKGROUND && body_index_map_data[i+width]!= K4ABT_BODY_INDEX_MAP_BACKGROUND && body_index_map_data[i+width+1]!= K4ABT_BODY_INDEX_MAP_BACKGROUND)) {
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

    k4abt_frame_release(body_frame);
    k4a_image_release(body_index_map);

    const std::string& filename = "test";
    std::ofstream myfile;
    std::stringstream fullname;
    fullname << filename << "_" << ".obj";
    myfile.open(fullname.str());

    std::cout << points.size() << std::endl;
    
    //vertex 좌표 정보
    for (size_t i = 0; i < points.size(); ++i) {
        myfile << "v ";
        myfile << points[i].xyz[0] << " " << points[i].xyz[1] << " " << points[i].xyz[2];
        myfile << "\n";
    }
    //texture 정보
    for (int i = 0; i < points.size(); i++) {
        myfile << "vt ";
        myfile << (float)(points[i].xth-left) / (right-left) << " " << (float)(points[i].yth - top) / (bottom-top);
        myfile << "\n";
    }

    //face 정보
    for (int i = 0; i < triangle.size()/3; i++) {
        myfile << "f ";
        myfile << triangle[3 * i] << "/" << triangle[3*i] << " ";
        myfile << triangle[3 * i + 1] << "/" << triangle[3 * i + 1] << " ";
        myfile << triangle[3 * i + 2] << "/" << triangle[3 * i + 2];
        myfile << "\n";
    }

    //Joint 좌표 정보(가운데 골반)
    myfile << "b ";
    myfile << (float)skeleton.joints[0].position.xyz.x << " " << (float)skeleton.joints[0].position.xyz.y << " " << (float)skeleton.joints[0].position.xyz.z;
    myfile << "\n";

    //왼쪽 엉덩이
    myfile << "b ";
    myfile << (float)skeleton.joints[18].position.xyz.x << " " << (float)skeleton.joints[18].position.xyz.y << " " << (float)skeleton.joints[18].position.xyz.z;
    myfile << "\n";

    //오른쪽 엉덩이
    myfile << "b ";
    myfile << (float)skeleton.joints[22].position.xyz.x << " " << (float)skeleton.joints[22].position.xyz.y << " " << (float)skeleton.joints[22].position.xyz.z;
    myfile << "\n";

    //왼쪽 발목
    myfile << "b ";
    myfile << (float)skeleton.joints[20].position.xyz.x << " " << (float)skeleton.joints[20].position.xyz.y << " " << (float)skeleton.joints[20].position.xyz.z;
    myfile << "\n";

    //오른쪽 발목
    myfile << "b ";
    myfile << (float)skeleton.joints[24].position.xyz.x << " " << (float)skeleton.joints[24].position.xyz.y << " " << (float)skeleton.joints[24].position.xyz.z;
    myfile << "\n";

    //가운데 골반
    myfile << "b ";
    myfile << (float)skeleton.joints[0].position.xyz.x << " " << (float)skeleton.joints[0].position.xyz.y << " " << (float)skeleton.joints[0].position.xyz.z;
    myfile << "\n";

    //목
    myfile << "b ";
    myfile << (float)skeleton.joints[3].position.xyz.x << " " << (float)skeleton.joints[3].position.xyz.y << " " << (float)skeleton.joints[3].position.xyz.z;
    myfile << "\n";

    //왼쪽 손목
    myfile << "b ";
    myfile << (float)skeleton.joints[7].position.xyz.x << " " << (float)skeleton.joints[7].position.xyz.y << " " << (float)skeleton.joints[7].position.xyz.z;
    myfile << "\n";

    //오른쪽 손목
    myfile << "b ";
    myfile << (float)skeleton.joints[14].position.xyz.x << " " << (float)skeleton.joints[14].position.xyz.y << " " << (float)skeleton.joints[14].position.xyz.z;
    myfile << "\n";

    k4a_device_close(device);
    myfile.close();
    
    return 0;
}

/*
#include <assert.h>
#include <k4a/k4a.h>
#include <k4abt.h>
#include <math.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

struct point {
    float xyz[3];
    int number;
};

static void create_xy_table(const k4a_calibration_t* calibration, k4a_image_t xy_table)
{
    k4a_float2_t* table_data = (k4a_float2_t*)(void*)k4a_image_get_buffer(xy_table);

    int width = calibration->depth_camera_calibration.resolution_width;
    int height = calibration->depth_camera_calibration.resolution_height;

    k4a_float2_t p;  //source point 2d
    k4a_float3_t ray; //target point 3d
    int valid;

    for (int y = 0, idx = 0; y < height; y++)
    {
        p.xy.y = (float)y;
        for (int x = 0; x < width; x++, idx++)
        {
            p.xy.x = (float)x;

            //깊이값이 있는 2d좌표를 3d 점으로 반환
            //p: 소스 카메라 coordinate 픽셀
            //ray: 입력 픽셀의 3d 좌표가 mm단위로 저장된 출력 포인터
            k4a_calibration_2d_to_3d(
                calibration, &p, 1.f, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &ray, &valid);

            if (valid)
            {
                table_data[idx].xy.x = ray.xyz.x;
                table_data[idx].xy.y = ray.xyz.y;
            }
            else
            {
                table_data[idx].xy.x = nanf("");
                table_data[idx].xy.y = nanf("");
            }
        }
    }
}

static void generate_point_cloud(const k4a_image_t depth_image, const k4a_image_t xy_table, k4a_image_t point_cloud, int* point_count, std::vector<float>& triangle)
{
    int width = k4a_image_get_width_pixels(depth_image);
    int height = k4a_image_get_height_pixels(depth_image);

    uint16_t* depth_data = (uint16_t*)(void*)k4a_image_get_buffer(depth_image);
    k4a_float2_t* xy_table_data = (k4a_float2_t*)(void*)k4a_image_get_buffer(xy_table);
    k4a_float3_t* point_cloud_data = (k4a_float3_t*)(void*)k4a_image_get_buffer(point_cloud);

    *point_count = 0;
    for (int i = 0; i < width * height; i++)
    {
        if (depth_data[i] != 0 && !isnan(xy_table_data[i].xy.x) && !isnan(xy_table_data[i].xy.y))
        {
            point_cloud_data[i].xyz.x = xy_table_data[i].xy.x * (float)depth_data[i];
            point_cloud_data[i].xyz.y = xy_table_data[i].xy.y * (float)depth_data[i];
            point_cloud_data[i].xyz.z = (float)depth_data[i];
            (*point_count)++;
        }
        else
        {
            point_cloud_data[i].xyz.x = nanf("");
            point_cloud_data[i].xyz.y = nanf("");
            point_cloud_data[i].xyz.z = nanf("");
        }
    }
}


static void write_point_cloud(const char* file_name, const k4a_image_t point_cloud, int point_count, const k4abt_skeleton_t& skeleton, std::vector<float> triangle)
{
    int width = k4a_image_get_width_pixels(point_cloud);
    int height = k4a_image_get_height_pixels(point_cloud);

    k4a_float3_t* point_cloud_data = (k4a_float3_t*)(void*)k4a_image_get_buffer(point_cloud);

    // save to the ply file
    //std::ofstream ofs(file_name); // text mode first
    //ofs << "ply" << std::endl;
    //ofs << "format ascii 1.0" << std::endl;
    //ofs << "element vertex" << " " << point_count << std::endl;
    //ofs << "property float x" << std::endl;
    //ofs << "property float y" << std::endl;
    //ofs << "property float z" << std::endl;
    //ofs << "end_header" << std::endl;
    //ofs.close();

    std::stringstream ss;
    ss << point_count << "\n";

    for (int i = 0; i < width * height; i++)
    {
        //데이터가 숫자가 아니면 continue
        if (isnan(point_cloud_data[i].xyz.x) || isnan(point_cloud_data[i].xyz.y) || isnan(point_cloud_data[i].xyz.z))
        {
            continue;
        }

        ss << "v " << (float)point_cloud_data[i].xyz.x << " " << (float)point_cloud_data[i].xyz.y << " "
            << (float)point_cloud_data[i].xyz.z << std::endl;
    }
    
    for (int i = 0; i < K4ABT_JOINT_COUNT; i++) {
        ss << "b " << (float)skeleton.joints[i].position.xyz.x << " " << (float)skeleton.joints[i].position.xyz.y << " "
            << (float)skeleton.joints[i].position.xyz.z << std::endl;
    }
    
    std::vector<int>triangle;

    for(int i=0;i<width*height;i++){
        if (!isnan(point_cloud_data[i].xyz.z) && !isnan(point_cloud_data[i+1].xyz.z && !isnan(point_cloud_data[i+width].xyz.z))
        {
            triangle.push_back();
        }
    }


    std::ofstream ofs_text(file_name, std::ios::out | std::ios::app);
    ofs_text.write(ss.str().c_str(), (std::streamsize)ss.str().length());
    ofs_text.close();
}


void print_body_information(k4abt_body_t body)
{
    printf("Body ID: %u\n", body.id);
    for (int i = 0; i < (int)K4ABT_JOINT_COUNT; i++)
    {
        k4a_float3_t position = body.skeleton.joints[i].position;
        k4a_quaternion_t orientation = body.skeleton.joints[i].orientation;
        k4abt_joint_confidence_level_t confidence_level = body.skeleton.joints[i].confidence_level;
        printf("Joint[%d]: Position[mm] ( %f, %f, %f ); Orientation ( %f, %f, %f, %f); Confidence Level (%d) \n",
            i, position.v[0], position.v[1], position.v[2], orientation.v[0], orientation.v[1], orientation.v[2], orientation.v[3], confidence_level);
    }
}

void print_body_index_map_middle_line(k4a_image_t body_index_map)
{
    uint8_t* body_index_map_buffer = k4a_image_get_buffer(body_index_map);

    // Given body_index_map pixel type should be uint8, the stride_byte should be the same as width
    // TODO: Since there is no API to query the byte-per-pixel information, we have to compare the width and stride to
    // know the information. We should replace this assert with proper byte-per-pixel query once the API is provided by
    // K4A SDK.
    assert(k4a_image_get_stride_bytes(body_index_map) == k4a_image_get_width_pixels(body_index_map));

    int middle_line_num = k4a_image_get_height_pixels(body_index_map) / 2;
    body_index_map_buffer = body_index_map_buffer + middle_line_num * k4a_image_get_width_pixels(body_index_map);

    printf("BodyIndexMap at Line %d:\n", middle_line_num);
    for (int i = 0; i < k4a_image_get_width_pixels(body_index_map); i++)
    {
        printf("%u, ", *body_index_map_buffer);
        body_index_map_buffer++;
    }
    printf("\n");
}

int main(int argc, char** argv)
{
    int returnCode = 1;
    k4a_device_t device = NULL;
    const int32_t TIMEOUT_IN_MS = 1000;
    k4a_capture_t capture = NULL;
    std::string file_name;
    uint32_t device_count = 0;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_image_t depth_image = NULL;
    k4a_image_t xy_table = NULL;
    k4a_image_t point_cloud = NULL;
    k4a_image_t body_index_map = NULL;

    k4abt_tracker_t tracker = NULL;
    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;

    int point_count = 0;
  
    if (argc != 2)
    {
        printf("Kinect.exe <output file>\n");
        returnCode = 2;
        
        if (device != NULL)
        {
            k4a_device_close(device);
        }
        return returnCode;
    }

    file_name = argv[1];

    device_count = k4a_device_get_installed_count();

    if (device_count == 0)
    {
        printf("No K4A devices found\n");
        return 0;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &device))
    {
        printf("Failed to open device\n");
        if (device != NULL)
        {
            k4a_device_close(device);
        }

        return returnCode;
    }

    config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    k4a_calibration_t calibration;

    if (K4A_RESULT_SUCCEEDED != k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
    {
        printf("Failed to get calibration\n");
        if (device != NULL)
        {
            k4a_device_close(device);
        }

        return returnCode;
    }

    
    k4abt_tracker_create(&calibration, tracker_config, &tracker);

    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
        calibration.depth_camera_calibration.resolution_width,
        calibration.depth_camera_calibration.resolution_height,
        calibration.depth_camera_calibration.resolution_width * (int)sizeof(k4a_float2_t),
        &xy_table);

    create_xy_table(&calibration, xy_table);

    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
        calibration.depth_camera_calibration.resolution_width,
        calibration.depth_camera_calibration.resolution_height,
        calibration.depth_camera_calibration.resolution_width * (int)sizeof(k4a_float3_t),
        &point_cloud);

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config))
    {
        printf("Failed to start cameras\n");
        if (device != NULL)
        {
            k4a_device_close(device);
        }

        return returnCode;
    }

    // Get a capture
    switch (k4a_device_get_capture(device, &capture, TIMEOUT_IN_MS))
    {
    case K4A_WAIT_RESULT_SUCCEEDED:
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        printf("Timed out waiting for a capture\n");
        if (device != NULL)
        {
            k4a_device_close(device);
        }
        return returnCode;
    case K4A_WAIT_RESULT_FAILED:
        if (device != NULL)
        {
            k4a_device_close(device);
        }
        return returnCode;
    }

    // Get a depth image
    depth_image = k4a_capture_get_depth_image(capture);
    if (depth_image == 0)
    {
        printf("Failed to get depth image from capture\n");
        if (device != NULL)
        {
            k4a_device_close(device);
        }

        return returnCode;
    }

    k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(tracker, capture, K4A_WAIT_INFINITE);
    k4a_capture_release(capture);
    
    if (queue_capture_result == K4A_WAIT_RESULT_FAILED) {
        printf("Error! Adding capture to tracker process queue failed!\n");
        return 0;
    }

    k4abt_frame_t body_frame = NULL;
    k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(tracker, &body_frame, K4A_WAIT_INFINITE);
    k4abt_skeleton_t skeleton;

    body_index_map = k4abt_frame_get_body_index_map(body_frame);

    if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED) {
        size_t num_bodies = k4abt_frame_get_num_bodies(body_frame);
        printf("%zu bodies are detected!\n", num_bodies);

        for (size_t i = 0; i < num_bodies; i++) {
            k4abt_frame_get_body_skeleton(body_frame, i, &skeleton);
            for (int j = 0; j < K4ABT_JOINT_COUNT; j++) {
                printf("x y z: %f %f %f\n", skeleton.joints[j].position.xyz.x,
                    skeleton.joints[j].position.xyz.y, skeleton.joints[j].position.xyz.z);
            }
            uint32_t id = k4abt_frame_get_body_id(body_frame, i);
        }

        k4abt_frame_release(body_frame);
    }
    k4a_image_release(body_index_map);
    std::vector<float> triangle;
    generate_point_cloud(depth_image, xy_table, point_cloud, &point_count,triangle);

    write_point_cloud(file_name.c_str(), point_cloud, point_count, skeleton, triangle);

    k4a_image_release(depth_image);
    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);
    k4a_image_release(xy_table);
    k4a_image_release(point_cloud);
    k4a_device_stop_cameras(device);
    k4a_device_close(device);
    
    returnCode = 0;

    return returnCode;
}

*/