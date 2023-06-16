# SketchQuiz

## 실행방법
```
// Compile cmd
$ git clone https://github.com/SuhyunRim118/SketchQuiz.git
$ cd SketchQuiz/src/final

$ gcc client.c -o client -lpthread
$ gcc server.c -o server -lpthread
```

## 프로그램 설명
프로젝트 이름은 SketchQuiz로, 서버-클라이언트 구조입니다.

여러 클라이언트가 접속할 수 있으며, TCP 전송 프로토콜을 사용합니다. 

그림판은 OpenCV를 사용해서 구현했고 개발환경은 Ubuntu 20.04.5입니다.

1. "start"입력 시 게임이 시작하고, 서버로부터 키워드가 주어집니다. 
2. 한 플레이어가 그림을 그리면 다른 플레이어들은 cmd에 정답을 입력해서 맞춥니다.
3. 정답을 맞추면 turn이 넘어가고, 유저 전부가 2번씩 문제를 내면 게임을 종료합니다.

## 동작 화면
[동작 화면 캡처본]
