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
    char viewtype;
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


directoryNode* find_directory(directoryTree* structure, char* name)
{
  directoryNode* temp = structure -> current -> firstchild;

  while (temp != NULL)
  {
    if (strcmp(temp -> name, name) == 0)
    {
      break;
    }
    temp = temp -> nextSibling;
  }
  return temp;
}
// structure와 name을 입력 받아 하위 디렉토리를 확인하고 있다면 위치를 return해주는 함수

void mod_print(int chmodinfo)
{
  int temp;
  int div = 100;
  int divideinfo;

  for(int i=0; i<3; i++)
  {
    divideinfo = chmodinfo / div;
    chmodinfo = chmodinfo - divideinfo * div;
    div = div / 10;

    if (divideinfo == 0)
    {
      printf("---");
    }
    else if (divideinfo == 1)
    {
      printf("--x");
    }
    else if (divideinfo == 2)
    {
      printf("-w-");
    }
    else if (divideinfo == 3)
    {
      printf("-wx");
    }
    else if (divideinfo == 4)
    {
      printf("r--");
    }
    else if (divideinfo == 5)
    {
      printf("r-x");
    }
    else if (divideinfo == 6)
    {
      printf("rw-");
    }
    else if (divideinfo == 1)
    {
      printf("rwx");
    }
  }
  printf("\t");
}

// int 값으로 정해진 권한에 대한 정보를 문자열로 변환하여 출력해주는 함수
// 소유자, 그룹, 다른사용자 권한을 나타낸다

int treepreorder(directoryNode* directoryNode, int node_num)
{
  if (directoryNode != NULL)
  {
    node_num++;
    if (directoryNode -> firstchild == NULL)
    {
      node_num = treepreorder(directoryNode -> firstChild, node_num);
      node_num = treepreorder(directoryNode -> nextSibling, node_num);
    }
    else
    {
      temp = directoryNode -> nextSibling;
      while (temp != NULL)
      {
        node_num++;
        temp = temp -> nextSibling;
      }
      node_num = treepreorder(directoryNode -> firstchild, node_num);
      node_num = treepreorder(directoryNode -> nextSibling, node_num);
    }
  }
  return node_num;
}

// 트리 탐색하는 것으로 방문 횟수를 return 해주는 함수
// 동일 level 탐색하는 횟수도 포함되어 있어서 빼주어야함

int directorylink(directoryNode* directoryNode)
{
  int node_num = 0;
  node_num = treepreorder(directoryNode, 0);

  if (directoryNode -> nextSibling != NULL)
  {
    directoryNode* temp = directoryNode -> nextSibling;
    while (temp != NULL)
    {
      node_num--;
      if (temp -> firstChild != NULL)
      {
        directoryNode* newtemp = temp -> firstChild;
        whlie (newtemp != NULL)
        {
          node_num--;
          newtemp = newtemp -> firstChild;
        }
      }
      temp = temp -> nextSibling;
    }
  }
  printf("%d\t", node_num);
}

// treepreorder의 return 값을 받고 동일한 level의 노드를 탐색한 횟수를 빼주는 것으로
// 최종 링크 수를 return 해준다.

void ls(directoryTree* structure, int option) // option: 0 ls 1 -l 2 -a 3 -al
{
  directoryNode* current_flow = structure -> current;
  directoryNode* temp;
  directoryNode* directory_list[20]; // 존재하는 파일 디렉토리 담는 list
  int directory_list_num = 0;

  if(current_flow -> fisrtChild == NULL) // 비어있으면 아무것도 하지 않음
  {}
  else{ // firstchild에 값이 있으면 nextSibling에 대한 값들도 계속 추가
    temp = current_flow -> firstchild -> nextSibling;
    if(temp == NULL)
    {
      directory_list[directory_list_num++] = current_flow -> firstchild;
    }
    else
    {
      directory_list[directory_list_num++] = current_flow -> firstchild;

      while(temp != NULL)
      {
        directory_list[directory_list_num++] = temp;
        temp = temp -> nextSibling;
      }
    }
  }

  if (options == 0) // ls
  {
    int num = 0;
    while (num < directory_num)
    {
      if(directory_list[num] -> viewtype == 's')
        printf("%s ", directory_list[num] -> name);
      num++;
    }
    printf("\n");
  }
  else if(option == 1) // ls -l
  {
    int num = 0;
    while (num < directory_num)
    {
      if(directory_list[num] -> viewtype == 's' && directory_list[num] -> type == 'd')
      {
        printf("d");
        mod_print(directory_list[num] -> permission);
        directorylink(directory_list[num]);
        printf("4096\t");

        int Month = directory_list[num] -> Date -> month+1;
        int Day = directory_list[num] -> Date -> day;
        int Hour = directory_list[num] -> Date -> hour;
        int Minute = directory_list[num] -> Date -> minute;
        int Second = directory_list[num] -> Date -> seccond;
        printf("month: %d\tday: %d\t%02d: %02d: %02d\t", Month, Day, Hour, Minute, Second);
        printf("%s\n", directory_list[num] -> name);
      }
      else if(directory_list[num] -> viewtype == 's' && directory_list[num] -> type == '-')
      {
        printf("-");
        mod_print(directory_list[num] -> permission);
        printf("1\t");
        printf("%d\t", sizeof(directory_list[num] -> SIZE));
        int Month = directory_list[num] -> Date -> month+1;
        int Day = directory_list[num] -> Date -> day;
        int Hour = directory_list[num] -> Date -> hour;
        int Minute = directory_list[num] -> Date -> minute;
        int Second = directory_list[num] -> Date -> second;
        printf("month: %d\tday: %d\t%02d: %02d: %02d\t", Month, Day, Hour, Minute, Second);
        printf("%s\n", directory_list[num] -> name);
      }
      num++;
    }
    printf("\n");
  }
  else if (option == 2) // ls -a
  {
    int num = 0;
    while (num < directory_num)
    {
      printf("%s ", directory_list[num] -> name);
      num++;
    }
    printf("\n");
  }
  else // option == 3 ls -al
  {
    int num = 0;
    while (num < directory_num)
    {
      if (directory_list[num] -> type == 'd')
      {
        printf("d");
        mod_print(directory_list[num] -> permission);
        directorylinkprint(directory_list[num]);
        printf("4096\t");

        int Month = directory_list[num] -> Date -> month+1;
        int Day = directory_list[num] -> Date -> day;
        int Hour = directory_list[num] -> Date -> hour;
        int Minute = directory_list[num] -> Date -> minute;
        int Second = directory_list[num] -> Date -> seccond;
        printf("month: %d\tday: %d\t%02d: %02d: %02d\t", Month, Day, Hour, Minute, Second);
        printf("%s\n", directory_list[num] -> name);
      }
      else if (directory_list[num] -> type == '-')
      {
        printf("-");
        mod_print(directory_list[num] -> permission);
        printf("1\t");
        printf("%d\t", sizeof(directory_list[num] -> SIZE));
        int Month = directory_list[num] -> Date -> month+1;
        int Day = directory_list[num] -> Date -> day;
        int Hour = directory_list[num] -> Date -> hour;
        int Minute = directory_list[num] -> Date -> minute;
        int Second = directory_list[num] -> Date -> second;
        printf("month: %d\tday: %d\t%02d: %02d: %02d\t", Month, Day, Hour, Minute, Second);
        printf("%s\n", directory_list[num] -> name);
      }
      num++;
    }
    printf("\n");
  }
}

void ch_mod(directoryTree* structure, int mod_value, char* name)
{
  directoryNode* temp = find_directory(structure, name);
  if (temp != NULL)
  {
    temp -> permission = mod_value;
  }
  else
  {
    printf("chmod: No such file or directory\n");
  }
}
// 파일 위치 찾고 있으면 권환 병경해주고 아니면 그런 파일이나 디렉토리 없다는 출력

