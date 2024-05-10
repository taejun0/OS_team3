#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
// #include <pthread.h>

#define MAX_NAME 256
#define MAX_DIR 1024

// 날짜 정보 - 파일이나 폴더의 수정 시간, 접근 시간에서 사용
typedef struct date {
    int year;
    int month;
    int weekday;
    int day;
    int hour;
    int minute;
    int second;
} Date;

// 사용자와 그룹 ID
typedef struct id {
    int UID;
    int GID;
} ID;

// 파일과 폴더 권한 
typedef struct permission {
    int mode; // 접근 모드 (ex 700, 400, 777 등)
    int permission[9]; // mode를 0 혹은 1의 값으로 풀어서 저장하는 배열 
} Permission;

// 사용자 노드 - 이름, 사용자의 홈 디렉토리, id, 
typedef struct userNode {
    char name[MAX_NAME];
    char dir[MAX_DIR];
    ID id;
    // Date date;       사용자에 Date가 필요할까? 에 대한 고민 필요
    struct userNode *nextNode; //다음 user를 가리키는 포인터로 user 탐색에 필요
} UserNode;

// UserList로 사용자들을 모아놓음 
typedef struct userList {
    ID topId;
    UserNode *head;
    UserNode *tail;
    UserNode *current;
} UserList;

// 파일이나 디렉토리의 inode 정보들
typedef struct directoryNode {
    char name[MAX_NAME];
    char type; // -는 파일, d는 디렉토리, l은 link파일
    int SIZE;
    Permission permission;
    ID id;
	Date date;
	struct directoryNode *parent;       //부모 폴더
	struct directoryNode *firstChild;   //자식 폴더 및 파일
	struct directoryNode *nextSibling;  //형제 관계의 폴더 및 파일
} DirectoryNode;

// 파일 시스템의 구조 - DirectoryTree*로 파일 시스템에 접근
typedef struct directoryTree {
	DirectoryNode* root;
	DirectoryNode* current;
} DirectoryTree;

// stackNode
typedef struct stackNode {
	char name[MAX_NAME];
	struct stackNode *nextNode;
} StackNode;

// 경로 분석에 사용할 stack 정의
typedef struct stack {
	StackNode* topNode;
	int cnt;
} Stack;

// 멀티스레딩에서 사용할 threadTree - 파일 및 폴더 생성 단계에서 사용 변경 필요
// typedef struct threadTree {
//     DirectoryTree *threadTree;
//     char *fileName;
//     char *content;    //파일의 내용 저장
//     char *command;    //사용자가 입력한 명령어 저장 
//     char *usrName;    //파일이나 디렉토리의 소유자
//     int mode;         //접근 권한
//     int option;       //옵션 
// } ThreadTree;
