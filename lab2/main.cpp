#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

// nasm汇编
extern "C" {
	void asm_print(const char *, const int);
}

// 简称
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

// 强制按1字节对齐
#pragma pack (1)

// 引导扇区
struct BPB{
	u16 BPB_BytsPerSec;	// 每扇区字节数
	u8 	BPB_SecPerClus;	// 每簇占用的扇区数
	u16 BPB_RsvdSecCnt;	// Boot占用的扇区数
	u8 	BPB_NumFATs;	// FAT表的记录数
	u16 BPB_RootEntCnt;	// 最大根目录文件数
	u16 BPB_TotSec16;	// 逻辑扇区总数
	u8 	BPB_Media;		// 媒体描述符
	u16 BPB_FATSz16;	// 每个FAT占用扇区数
	u16 BPB_SecPerTrk;	// 每个磁道占用扇区数
	u16 BPB_NumHeads;	// 磁头数
	u32 BPB_HiddSec;	// 隐藏扇区数
	u32 BPB_TotSec32;	// 如果BPB_TotSec16是0，则在这记录扇区总数
};

// 根目录区 目录项
struct RootEntry{
	char	DIR_Name[11];	// 文件名8字节，扩展名3字节
	u8 		DIR_Attr;		// 文件属性
	char 	Reserve[10];	// 保留位
	u16 	DIR_WrtTime;	// 最后一次写入时间
	u16 	DIR_WrtDate;	// 最后一次写入日期
	u16 	DIR_FstClus;	// 此条目对应的开始簇数
	u32		DIR_FileSize;	// 文件大小
};

// 取消强制按1字节对齐
#pragma pack ()

// 全局变量
int BytsPerSec;
int SecPerClus;
int RsvdSecCnt;
int NumFATs;
int RootEntCnt;
int FATSz;

// 文件/目录节点
class ItemNode {
public:
	bool 	type = false;					// false为目录，true为文件
	string 	name;							// 名称
	string 	path;							// 路径
	u32 	size;							// 大小
	vector<ItemNode*> next;					// 下一级目录的Node数组
	int 	dirNum = 0; 					// 下一级的目录数
	int 	fileNum = 0; 					// 下一级的文件数
	char*	content = new char[10000]{};	// 若为文件，这里存放文件内容
	bool 	point = false; 					// false表示不是.或..，true表示是.或..
};

void readBPB(FILE* fat12, struct BPB* bpbPtr);
void readEntry(FILE* fat12, struct RootEntry* rootEntryPtr, ItemNode* node);
void readChildren(FILE* fat12, int startClus, ItemNode* node);
int getFATValue(FILE* fat12, int num);
void getContent(FILE* fat12, int startClus, ItemNode* node);
void printLs(ItemNode* node);
void printLs_l(ItemNode* node);
int printLs_path(ItemNode* node, string s);
int printLs_l_path(ItemNode* node, string s);
int printCat(ItemNode* node, string s);
void myprint(const char* ptr);
vector<string> split(const string& str, const string& delim);

int main(){
	// 打开FAT12镜像
	FILE* fat12;
	fat12 = fopen("a.img", "rb");

	// 读取BPB
	struct BPB bpb;
	struct BPB* bpbPtr = &bpb;
	readBPB(fat12, bpbPtr);

	// 给全局变量赋值
	BytsPerSec = bpb.BPB_BytsPerSec;
	SecPerClus = bpb.BPB_SecPerClus;
	RsvdSecCnt = bpb.BPB_RsvdSecCnt;
	NumFATs = bpb.BPB_NumFATs;
	RootEntCnt = bpb.BPB_RootEntCnt;
	if(bpb.BPB_FATSz16 != 0){
		FATSz = bpb.BPB_FATSz16;
	}
	else{
		FATSz = bpb.BPB_TotSec32; 
	}

	// 读取BPB
	struct RootEntry rootEntry;
	struct RootEntry* rootEntryPtr = &rootEntry;

	// 根目录节点
	ItemNode *root = new ItemNode();
	root->name = "";
	root->path = "/";

	// 读取并构建文件结构
	readEntry(fat12, rootEntryPtr, root);
	
	// 读取输入并给出反应
	while(true){
		// 读取并处理输入
		string input;
		myprint(">");
		getline(cin, input);
		vector<string> inputList = split(input, " ");

		// 退出
		if (inputList[0].compare("q") == 0){
			myprint("quit\n");
			break;
		}
		// ls
		else if(inputList[0].compare("ls") == 0){
			// 单ls
			if(inputList.size() == 1){
				printLs(root);
			}
			// 带参数
			else{
				bool has_l = false;
				bool hasPath = false;
				bool flag = true;
				string* path = NULL;
				for(int i = 1; i < inputList.size(); ++i){
					// -l
					if(inputList[i].length() > 0 && inputList[i][0] == '-'){
						if(inputList[i].length() >= 2){
							for (int j = 1; j < inputList[i].length(); ++j)
							{
								if(inputList[i][j] != 'l'){
									myprint("wrong parameter!\n");
									flag = false;
									break;
								}
							}
							if(!flag){
								break;
							}
							has_l = true;
						}
						else{
							myprint("wrong parameter!\n");
							flag = false;
							break;
						}
					}
					// path
					else{
						if(!hasPath){
							hasPath = true;
							if(inputList[i].length() == 0){
								myprint("invalid path!\n");
								flag = false;
								break;
							}
							if (inputList[i][0] != '/') {
								inputList[i] = "/" + inputList[i];
							}
							if (inputList[i][inputList[i].length() - 1] != '/') {
								inputList[i] += '/';
							}
							path = &inputList[i];
						}
						else{
							myprint("too many path!\n");
							flag = false;
							break;
						}
					}
				}
				// 不支持
				if(!flag){
					continue;
				}

				int valid = 0;

				// ls -l
				if(has_l && !hasPath){
					valid = 1;
					printLs_l(root);
				}
				// ls path
				else if(!has_l && hasPath){
					valid = printLs_path(root, *path);
				}
				// ls -l path
				else if(has_l && hasPath){
					valid = printLs_l_path(root, *path);
				}
				else{
					myprint("invaild command!\n");
					continue;
				}

				if(valid == 0){
					myprint("no such path!\n");
				}
				else if(valid == 2){
					myprint("can not open!\n");
				}
			}
		}
		else if(inputList[0].compare("cat") == 0){
			// cat
			if(inputList.size() <= 1){
				myprint("no parameter wrong!\n");
			}
			// cat path
			else if (inputList.size() == 2 && inputList[1][0]!='-') {
				int valid = 0;
				if (inputList[1][0] != '/') {
					inputList[1] = "/" + inputList[1];
				}
				if (inputList[1][inputList[1].length() - 1] != '/') {
					inputList[1] += '/';
				}
				valid = printCat(root, inputList[1]);
				if (valid == 0) {
					myprint("no such path!\n");
				}
				else if (valid == 2) {
					myprint("can not open!\n");
				}
			}
			// cat ?
			else if(inputList.size() == 2 && inputList[1][0]=='-'){
				myprint("wrong parameter!\n");
			}
			else{
				myprint("too many parameter!\n");
			}
		}
		else{
			// 错误的指令
			myprint("command error!\n");
		}
	}

	fclose(fat12);
	return 0;
}

void readBPB(FILE* fat12, struct BPB* bpbPtr){
	int ok = 0;
	ok = fseek(fat12, 11, SEEK_SET);
	if(ok == -1){
		myprint("readBPB: fseek failed!\n");
	}
	ok = fread(bpbPtr, 1, 25, fat12);
	if(ok != 25){
		myprint("readBPB: freadss failed!\n");
	}
	return;
}

void readEntry(FILE* fat12, struct RootEntry* rootEntryPtr, ItemNode* node){
	int ok = 0;
	// 存名字
	char temp[12];

	// 计算首字节偏移
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;

	// 处理根目录里的项
	for(int i = 0; i < RootEntCnt; i++){
		ok = fseek(fat12, base, SEEK_SET);
		if(ok == -1){
			myprint("readEntry: fseek failed!\n");
		}
		ok = fread(rootEntryPtr, 1, 32, fat12);
		if(ok != 32){
			myprint("readEntry: freadss failed!\n");
		}
		base += 32;

		// 跳过空的项
		if(rootEntryPtr->DIR_Name[0] == '\0'){
			continue;
		}

		// 跳过不关心的项
		bool validName = true;
		for (int j = 0; j < 11; j++) {
			// 不是英文字母、数字、空格
			if (!(((rootEntryPtr->DIR_Name[j] >= 'a') && (rootEntryPtr->DIR_Name[j] <= 'z')) ||
				((rootEntryPtr->DIR_Name[j] >= 'A') && (rootEntryPtr->DIR_Name[j] <= 'Z')) ||
				((rootEntryPtr->DIR_Name[j] >= '0') && (rootEntryPtr->DIR_Name[j] <= '9')) ||
				(rootEntryPtr->DIR_Name[j] == ' '))) {
				validName = false;
				break;
			}
		}
		if (!validName){
			continue;
		}

		// 处理文件/目录的名字
		if ((rootEntryPtr->DIR_Attr & 0x10) == 0) {
			// 文件
			int length = -1;
			for (int j = 0; j < 11; j++) {
				if (rootEntryPtr->DIR_Name[j] != ' ') {
					length++;
					temp[length] = rootEntryPtr->DIR_Name[j];
				}
				else {
					length++;
					temp[length] = '.';
					while (j < 10 && rootEntryPtr->DIR_Name[j + 1] == ' '){
						j++;
					}
				}
			}
			length++;
			temp[length] = '\0';
			// 新建文件节点
			ItemNode *son = new ItemNode();
			son->type = true;
			son->name = temp;
			son->path = node->path + temp + "/";
			son->size = rootEntryPtr->DIR_FileSize;
			node->next.push_back(son);
			node->fileNum++;
			getContent(fat12, rootEntryPtr->DIR_FstClus, son);
		}
		else {
			// 目录
			int length = -1;
			for (int j = 0; j < 11; j++) {
				if (rootEntryPtr->DIR_Name[j] != ' ') {
					length++;
					temp[length] = rootEntryPtr->DIR_Name[j];
				}
				else {
					length++;
					temp[length] = '\0';
					break;
				}
			}
			// 新建目录节点
			ItemNode *son = new ItemNode();
			son->name = temp;
			son->path = node->path + temp + "/";

			// . and ..
			ItemNode *point = new ItemNode();
			point->name = ".";
			point->point = true;
			son->next.push_back(point);
			ItemNode *doublePoint = new ItemNode();
			doublePoint->name = "..";
			doublePoint->point = true;
			son->next.push_back(doublePoint);

			node->next.push_back(son);
			node->dirNum++;
			readChildren(fat12, rootEntryPtr->DIR_FstClus, son);
		}
	}
}

void readChildren(FILE* fat12, int startClus, ItemNode* node){
	// 数据区第一个簇（2号簇）的偏移字节量
	int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

	int currentClus = startClus;
	// value用来查看是否存在多个簇（查FAT表）
	int value = 0;
	while (value < 0xFF8) {
		// 查FAT表获取下一个簇号
		value = getFATValue(fat12, currentClus);
		if (value == 0xFF7) {
			myprint("bad cluster!\n");
			break;
		}

		int startByte = base + (currentClus - 2) * SecPerClus*BytsPerSec;
		int ok;

		// 每簇的字节数
		int size = SecPerClus * BytsPerSec;
		int count = 0;
		while (count < size) {
			// 读取目录项
			RootEntry sonEntry;
			RootEntry *sonEntryPtr = &sonEntry;
			ok = fseek(fat12, startByte + count, SEEK_SET);
			if (ok == -1)
				myprint("readChildren: fseek failed!\n");

			ok = fread(sonEntryPtr, 1, 32, fat12);
			if (ok != 32)
				myprint("readChildren: fread failed!\n");
			count += 32;
			// 跳过空的项
			if (sonEntryPtr->DIR_Name[0] == '\0') {
				continue;
			}

			// 跳过不关心的项
			bool validName = true;
			for (int i = 0; i < 11; i++) {
				// 不是英文字母、数字、空格
				if (!(((sonEntryPtr->DIR_Name[i] >= 'a') && (sonEntryPtr->DIR_Name[i] <= 'z')) ||
				((sonEntryPtr->DIR_Name[i] >= 'A') && (sonEntryPtr->DIR_Name[i] <= 'Z')) ||
				((sonEntryPtr->DIR_Name[i] >= '0') && (sonEntryPtr->DIR_Name[i] <= '9')) ||
				(sonEntryPtr->DIR_Name[i] == ' '))) {
					validName = false;
					break;
				}
			}
			if (!validName) {
				continue;
			}


			if ((sonEntryPtr->DIR_Attr & 0x10) == 0) {
				// 处理文件
				// 存名字
				char temp[12];
				int length = -1;
				for (int i = 0; i < 11; i++) {
					if (sonEntryPtr->DIR_Name[i] != ' ') {
						length++;
						temp[length] = sonEntryPtr->DIR_Name[i];
					}
					else {
						length++;
						temp[length] = '.';
						while (i < 10 && sonEntryPtr->DIR_Name[i + 1] == ' '){
							i++;
						}
					}
				}
				length++;
				temp[length] = '\0';
				ItemNode *son = new ItemNode();
				node->next.push_back(son);
				son->name = temp;
				son->size = sonEntryPtr->DIR_FileSize;
				son->type = true;
				son->path = node->path + temp + "/";
				node->fileNum++;
				getContent(fat12, sonEntryPtr->DIR_FstClus, son);

			}
			else {
				char temp[12];
				int length = -1;
				for (int i = 0; i < 11; i++) {
					if (sonEntryPtr->DIR_Name[i] != ' ') {
						length++;
						temp[length] = sonEntryPtr->DIR_Name[i];
					}
					else {
						length++;
						temp[length] = '\0';
					}
				}

				ItemNode *son = new ItemNode();
				node->next.push_back(son);
				son->name = temp;
				son->path = node->path + temp + "/";
				node->dirNum++;

				ItemNode *point = new ItemNode();
				point->name = ".";
				point->point = true;
				son->next.push_back(point);
				ItemNode *doublePoint = new ItemNode();
				doublePoint->name = "..";
				doublePoint->point = true;
				son->next.push_back(doublePoint);

				readChildren(fat12, sonEntryPtr->DIR_FstClus, son);
			}

		}

		// 下一个簇
		currentClus = value;
	}
}

int getFATValue(FILE* fat12, int num) {
	// FAT1的偏移字节
	int base = RsvdSecCnt * BytsPerSec;
	// FAT项的偏移字节
	int position = base + num * 3 / 2;

	// 先读FAT项所在的两个字节
	u16 twoBytes;
	u16* twobytesPtr = &twoBytes;
	int ok;
	ok = fseek(fat12, position, SEEK_SET);
	if (ok == -1)
		myprint("getFATValue: fseek failed!\n");

	ok = fread(twobytesPtr, 1, 2, fat12);
	if (ok != 2)
		myprint("getFATValue: fread failed!\n");

	// 奇偶FAT项处理方式不同，分类处理
	if (num % 2 == 0) {
		twoBytes = twoBytes << 4;
		return twoBytes >> 4;
	}
	else {
		return twoBytes >> 4;
	}
}

void getContent(FILE* fat12, int startClus, ItemNode* node) {
	// 获取文件内容
	int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
	int currentClus = startClus;
	// 用value进行不同簇的读取（超过512字节）
	int value = 0;
	char* p = node->content;
	if (startClus == 0) {
		return;
	}
	while (value < 0xFF8) {
		//获取下一个簇
		value = getFATValue(fat12, currentClus);
		if (value == 0xFF7){
			myprint("bad cluster!\n");
			break;
		}
		// 存从簇中读出的数据
		int length = SecPerClus * BytsPerSec;
		char* str = (char*)malloc(length);
		char* content = str;
		int startByte = base + (currentClus - 2) * length;

		int ok;
		ok = fseek(fat12, startByte, SEEK_SET);
		if (ok == -1)
			myprint("getContent: fseek failed!");

		ok = fread(content, 1, length, fat12);
		if (ok != length)
			myprint("getContent: fread failed!");

		// 读取并赋值
		for (int i = 0; i < length; i++) {
			*p = content[i];
			p++;
		}
		free(str);
		currentClus = value;
	}
}

void printLs(ItemNode* node) {
	ItemNode* p = node;
	if (p->type) {
		// 文件
		return;
	}
	else {
		string str_print;
		str_print = p->path + ":\n";
		myprint(str_print.c_str());
		str_print.clear();
		// 打印下属节点
		ItemNode *q;
		int length = p->next.size();
		for (int i = 0; i < length; i++) {
			q = p->next[i];
			if (q->type == false) {
				// 目录
				str_print = "\033[31m" + q->name + "\033[0m" + "  ";
				myprint(str_print.c_str());
				str_print.clear();
			}
			else {
				// 文件
				str_print = q->name + "  ";
				myprint(str_print.c_str());
				str_print.clear();
			}
		}
		myprint("\n");
		// 递归
		for (int i = 0; i < length; i++) {
			if (p->next[i]->point == false){
				printLs(p->next[i]);
			}
		}
	}
}

void printLs_l(ItemNode *node) {
	ItemNode* p = node;
	if (p->type) {
		// 文件
		return;
	}
	else {
		string str_print;
		str_print = p->path + " " + to_string(p->dirNum) + " " + to_string(p->fileNum) + ":\n";
		myprint(str_print.c_str());
		str_print.clear();
		// 打印下属节点
		ItemNode *q;
		int length = p->next.size();
		for (int i = 0; i < length; i++) {
			q = p->next[i];
			if (q->type == false) {
				// 目录
				if (!(q->point)) {
					str_print = "\033[31m" + q->name + "\033[0m" + "  " + to_string(q->dirNum) + " " + to_string(q->fileNum) + "\n";
					myprint(str_print.c_str());
					str_print.clear();
				}
				else {
					// . and ..
					str_print = "\033[31m" + q->name + "\033[0m" + "\n";
					myprint(str_print.c_str());
					str_print.clear();
				}
			}
			else {
				// 文件
				str_print = q->name + "  " + to_string(q->size) + "\n";
				myprint(str_print.c_str());
				str_print.clear();
			}
		}
		myprint("\n");
		// 递归
		for (int i = 0; i < length; i++) {
			// not . or ..
			if (!(p->next[i]->point)){
				printLs_l(p->next[i]);
			}
		}
	}
}

int printLs_path(ItemNode *node, string s) {
	if (s.length() < node->path.length()) {
		// 目标路径长度短于当前路径，不可能找到
		return 0;
	}
	if (s.compare(node->path) == 0) {
		// 路径相同，找到
		if (node->type) {
			// 文件
			return 2;
		}
		else {
			// 目录
			printLs(node);
			return 1;
		}
	}
	// 是否在当前目录下
	string temp = s.substr(0, node->path.length());
	if (temp.compare(node->path) == 0) {
		// 递归
		for (ItemNode* p : node->next) {
			int temp = printLs_path(p, s);
			if(temp > 0){
				return temp;
			}
		}
	}
	return 0;
}

int printLs_l_path(ItemNode *node, string s) {
	if (s.length() < node->path.length()) {
		// 目标路径长度短于当前路径，不可能找到
		return 0;
	}
	if (s.compare(node->path) == 0) {
		// 路径相同，找到
		if (node->type) {
			// 文件
			return 2;
		}
		else {
			// 目录
			printLs_l(node);
			return 1;
		}
	}
	// 是否在当前目录下
	string temp = s.substr(0, node->path.length());
	if (temp.compare(node->path) == 0) {
		// 递归
		for (ItemNode* p : node->next) {
			int temp = printLs_l_path(p, s);
			if(temp > 0){
				return temp;
			}
		}
	}
	return 0;
}

int printCat(ItemNode *node, string s) {
	if (s.length() < node->path.length()) {
		// 目标路径长度短于当前路径，不可能找到
		return 0;
	}
	if (s.compare(node->path) == 0) {
		// 路径相同，找到
		if (node->type) {
			// 文件
			if (node->content[0] != 0) {
				myprint(node->content);
				myprint("\n");
			}
			return 1;
		}
		else {
			// 目录
			return 2;
		}
	}
	// 是否在当前目录下
	string temp = s.substr(0, node->path.length());
	if (temp.compare(node->path) == 0) {
		// 递归
		for (ItemNode* p : node->next) {
			int temp = printCat(p, s);
			if(temp > 0){
				return temp;
			}
		}
	}
	return 0;
}

void myprint(const char* ptr){
	asm_print(ptr, strlen(ptr));
}

vector<string> split(const string& str, const string& delim){
	vector<string> res;  
	if("" == str){
		return res;
	}  
	char* strs = new char[str.length() + 1];  
	strcpy(strs, str.c_str());   
 
	char* d = new char[delim.length() + 1];  
	strcpy(d, delim.c_str());  
 
	char* p = strtok(strs, d);  
	while(p) {  
		string s = p;  
		if(s.compare(" ") != 0 && s.compare("") != 0 && s.compare("\n") != 0){
			res.push_back(s);
		} 
		p = strtok(NULL, d);  
	}  
 
	return res;
}