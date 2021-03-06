# :dress: Capstone_Design_Team_Dress_Up :dress:

### :zero: 구성원 소개

1. :man_office_worker: 김대선 (경희대학교 컴퓨터공학과 2017103961 ) 
   소켓 서버 및 OpenGL
2. :man_student: 이준용 (경희대학교 컴퓨터공학과 2017104017) 
   키넥트 카메라 및 모델링

### :one: 프로젝트 소개

키넥트 카메라를 사용해 3D Point Cloud 프로세싱하여 OpenGL 라이브러리를 사용해 모델링 및 실감나게 렌더링 한다. 프로젝트 시나리오는 다음과 같다. 

1. 키넥트 카메라를 사용해서 대상을 촬영하고, 
2. 대상의 신체 정보를 좌표화하고 수치화 한다. 
3. 수치화된 좌표를 소켓 서버 - 클라이언트를 통해서 OpenGL 어플리케이션으로 송신한다. 
4. 받은 좌표를 통해서 기본적인 모델을 그린다. 
5.  사용자의 입력에 따라서 모델의 특정부분(상의, 하의)의 텍스쳐를 변경할 수 있다. 

이를 통해서 사용자는 오프라인 매장 등에서 옷을 직접 갈아 입지 않아도, 옷을 갈아입은 본인의 모습을 확인할 수 있다.

### :two: 예상 결론

1. 키넥트 카메라를 통해서 신체 정보를 수치화된 좌표로 변환하고 OpenGL을 통해서 해당 모델을 그릴 수 있다.
2. OpenGL을 통해서 해당 모델에 텍스쳐를 입힐 수 있다.
3. OpenGL을 통해서 텍스쳐를 입힐 때 3D Point 정보를 바탕으로 상의와 하의를 구분해서 입힐 수 있다.

### :three: 진행상황

1. :heavy_check_mark: 소켓 서버 구현
2. :heavy_check_mark: obj 파일 파싱해서 GLUT 그리기
3. :heavy_check_mark: OpenGL로 모델에 텍스쳐 입히기
4. :heavy_check_mark: 사진 촬영해서 정점 가져오기
5. :heavy_check_mark: 신체 관절좌표 가져오기
6. ✔️ 3D 모델링
7. ✔️ 모델에 일부분에 텍스쳐 입히기
8. ✔️ 사용자 입력에 따라 텍스쳐 변경하기
9. ✔️ triangulation

### :four: 선택가능한 패턴 종류

![선택가능한 패턴](https://user-images.githubusercontent.com/65288601/172844682-5950f197-5e43-4bad-a23f-6ad56dde47c5.PNG)

### :five: 사용 방법

1. OpenGL 디렉토리 코드를 빌드 후 실행한다
2. Kinect 디렉토리 코드를 빌드 후 실행한다.
3. Kinect 카메라가 연결되면 Console의 안내에 따라 진행한다.
4. Kinect 쪽에서 OpenGL 쪽으로 데이터를 전송한 후 OpenGL의 Console 안내에 따라 진행한다.

### :six: 결과물 예시

![도커티](https://user-images.githubusercontent.com/65288601/172844712-c44d93dd-4750-4057-b783-29c7ea5fa841.PNG)
