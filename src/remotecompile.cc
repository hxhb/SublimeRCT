#include "iniSetting.hpp"
#include <iostream>
#include <fstream>
#include <io.h>
#include <map>
#include <set>
#include <ctime>
#include <direct.h>
#include <windows.h>

using namespace iniFile;

#define MAXPATH 512

static iniSetting setting; // global .ini  setting file
static map<string,string> unionArgs; // args pack of .ini
static string binaryAbsPath;
static string nowTime;
// calculate string hash
static constexpr size_t HASH_STRING_PIECE(const char *string_piece, size_t hashNum = 0) {
	return *string_piece ? HASH_STRING_PIECE(string_piece + 1, (hashNum * 131) + *string_piece) : hashNum;
}
static constexpr size_t operator "" _HASH(const char *string_pice, size_t) {
	return HASH_STRING_PIECE(string_pice);
}
static size_t hashNum(const string &str) {
	return HASH_STRING_PIECE(str.c_str());
}

// check key->value match
bool checkIniKey(const std::string& key){
	const std::string section("RemoteCompileSSHSetting");
	int keyState=0;
	// cout<<::setting.getStringValue(section, key)<<endl;
	if(::setting.getStringValue(section, key, keyState).size()==0){
		return false;
	}else{
		unionArgs[key]=::setting.getStringValue(section, key, keyState);
		return true;
	}
}

// check have .ini file
bool checkHaveIniFile(const string& execPath){
	// cout<<execPath<<endl;
	if(!access(execPath.c_str(),F_OK)){
		// printf("have\n");
		return true;
	}else{
		// printf("no\n");
		return false;
	}
}

// if not found .ini file,create a .ini file
void newIniFile(const string& iniFilePath){
	fstream settingIni(iniFilePath,ofstream::out);
	settingIni<<"[RemoteCompileSSHSetting]\n;remote host addr(e.g root@192.168.137.2)\nhost=\n"<<endl
			  <<";remote host port(e.g:22)\nport=22\n\n;remote host user password\npassword=\n"<<endl
			  <<";sshKey(e.g:C:\\\\Users\\\\imzlp\\\\.ssh)\nsshKey=\n\n;compiler(e.g:gcc/clang)\ncompiler=gcc\n"<<endl
			  <<";e.g:-o/-o2\noptimizied=-o\n\n;std version e.g:c99/c11 or c++0x/c++11"<<endl
			  <<"stdver=-std=c++11\n\n;other compile args e.g:-pthread\notherCompileArgs=-pthread\n"<<endl
			  <<";remote host temp folder e.g:/tmp\n;It is recommended that you leave the default values(/tmp)\nremoteTempFolder=/tmp\n"<<endl
			  <<";Upload SourceFile to remote host path\n;auto create a name is localtime of folder\nuploadTo=/tmp\n"<<endl
			  <<";samba mapping to local DeskDrive\nsambaDrive=\n"<<endl
			  <<";remote sambda Path\nremoteSambaPath=";
	settingIni.close();
}

// merge args to one string
string mergeStr(initializer_list<string> x){
	string total;
	for(initializer_list<string>::iterator index=x.begin();index<x.end();++index){
		total+=*index;
		if(index!=x.end()-1){
			total+=" ";
		}
	}
	return total;
}

// conver char* to std::string
string converCharPtoStr(const char* z){
	string s;
	while(*z!='\0'){
		s.push_back(*z++);
	}
	return s;
}

// C:\test to C:\\test
void fixWinPathDoubleSprit(string& path){
	string temp;
	for(string::iterator beg=path.begin();beg!=path.end();++beg){
		if(*beg=='\\'){
			temp.push_back('\\');
			temp.push_back('\\');
		}else{
			temp.push_back(*beg);
		}
	}
	path=temp;
}

// get The exec programming fullPath
string getExecAbsPath(void){
	char buffer[MAXPATH];
	getcwd(buffer, MAXPATH);
	string execPath=converCharPtoStr(buffer);
	return execPath;
}

string getTheProgramAbsPath(void){
	TCHAR exeFullPath[MAX_PATH]; // MAX_PATH在WINDEF.h中定义了，等于260
	memset(exeFullPath,0,MAX_PATH);
	GetModuleFileName(NULL,exeFullPath,MAX_PATH);
	string binaryHaveBinaryNameAbsPath=converCharPtoStr(exeFullPath);
	// cout<<converCharPtoStr(exeFullPath)<<endl;
	string binaryAbsPath=string(binaryHaveBinaryNameAbsPath.begin(),binaryHaveBinaryNameAbsPath.begin()+binaryHaveBinaryNameAbsPath.find_last_of("\\"));
	// fixWinPathDoubleSprit(binaryAbsPath);
	// cout<<binaryAbsPath<<endl;
	return binaryAbsPath;
}
// get Source file prefix name
string getFilePrefix(const string& SourceFilePath){
	string temp(SourceFilePath.begin()+SourceFilePath.find_last_of('.')+1,SourceFilePath.end());
	// cout<<temp<<endl;
	return temp;
}

// match programming language
string matchLangCompiler(const string& filePrefix){
	set<string> cpplang={"cpp","cc","c++","hpp"};
	if(cpplang.find(filePrefix)!=cpplang.end()){
		return string("cpp");
	}else{
		return string("c");
	}
}
// get local host now time
string getNowTime(){
	time_t t = time(nullptr);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y%m%d-%X",localtime(&t));
    return converCharPtoStr(tmp);
}

// convert relative path to abs path
string getUplodeFullPath(const string& path){
	string remoteHostUserName(unionArgs["host"].begin(),unionArgs["host"].begin()+unionArgs["host"].find_first_of('@'));
	string uploadTofullPath;
	if(path.find('~')!=string::npos){
		uploadTofullPath="/home/"+remoteHostUserName+string(path.begin()+path.find('~')+1,path.end());
	}else{
		uploadTofullPath=path;
	}
	// cout<<uploadTofullPath<<endl;
	return uploadTofullPath;
}

void uploadOutMessage(const string &createNowTimeFolder,const string& nowTime){
	cout<<"Check whether there is "<<unionArgs["uploadTo"]<<"..."<<endl;
	cout<<"Create "<<unionArgs["uploadTo"]<<"/"<<nowTime<<"..."<<endl;
	system(createNowTimeFolder.c_str());
	cout<<"The folder "<<unionArgs["uploadTo"]<<"/"<<nowTime<<" was created successfully."<<endl;
}
string uploadTo(const string &sshPscp,const string& localfilePath,const string& UpMode){
	string pscp;
	switch(hashNum(UpMode)){
		case "file"_HASH: {
			pscp=mergeStr({"\""+sshPscp,"\""+localfilePath+"\"",unionArgs["host"]+":"+getUplodeFullPath(unionArgs["uploadTo"])});
			break;
		}
		case "folder"_HASH: {
			string localfileNoFileNamePath=string(localfilePath.begin(),localfilePath.begin()+localfilePath.find_last_of("\\"));
			pscp=mergeStr({"\""+sshPscp,"-r","\""+localfileNoFileNamePath+"\"",unionArgs["host"]+":"+getUplodeFullPath(unionArgs["uploadTo"])});
			break;
		}
		default: {
			cout<<"Parameter error!"<<endl;
		}
	}
	pscp+="/"+nowTime+"\"";
	return pscp;
}
string sambdaMapToLocalParse(string localSambdaPath){
	string samba;
	samba=getUplodeFullPath(unionArgs["remoteSambaPath"])+"/";
	for(auto &index:localSambdaPath){
		if(index=='\\'){
			index='/';
		}
	}
	samba+=string(localSambdaPath.begin()+localSambdaPath.find_first_of('/')+1,localSambdaPath.end());
	return samba;
}

void howuse(void){
	cout<<"Usage:\n\tTheProgram SourceFileFullPath RunMode"<<endl;
	cout<<"RunMode:\n  panelRun: \n\tRun on SublimeText Panel."<<endl
		<<"  terminalRun: \n\tRun on a new window."<<endl
		<<"  uploadThisFile: \n\tUpload current file to remoteHost."<<endl
		<<"  uploadThisFolderAndRun: \n\tUpload current folder to remoteHost And Run in SublimeText Panel."<<endl
		<<"  uploadCurrentFolderAndTerminalRun: \n\tUpload current folder to remoteHost And Run in NewWindow."<<endl
		<<"  cleanUpRemoteTemp: \n\tclean up remote host temp folder."<<endl
		<<"  openTerminal: \n\tOpen a SSH link to remoteHost."<<endl
		<<"E.g.:"<<endl
		<<"\tcommandExec D:\\TEST\\A.cc terminalRun"<<endl;
}

string sshBinaryMerge(const string& binaryName,const string& servedVerifyMode){
	string sshCommand;
	switch(hashNum(binaryName)){
		case "plink"_HASH: {
			sshCommand=mergeStr({"\""+binaryAbsPath+"\\SSHTools\\"+binaryName+"\"",unionArgs["host"],"-P",unionArgs["port"],servedVerifyMode});
			break;
		}
		case "pscp"_HASH: {
			sshCommand=mergeStr({"\""+binaryAbsPath+"\\SSHTools\\"+binaryName+"\"","-P",unionArgs["port"],servedVerifyMode});
			break;
		}
		case "putty"_HASH: {
			sshCommand=mergeStr({"\""+binaryAbsPath+"\\SSHTools\\"+binaryName+"\"",unionArgs["host"],"-P",unionArgs["port"],servedVerifyMode});
			break;
		}
		default:{
			cout<<"func sshBinaryMerge Args error!"<<endl;
		}
	}
	return sshCommand;
}
int main(int argc, char const *argv[])
{
	SetConsoleTitle("RemoteCompile");
	binaryAbsPath=getTheProgramAbsPath();
	nowTime=getNowTime();
	// cout<<binaryAbsPath<<endl;
	string localfilePath,localNoFileNamePath,filename,foldername,filenameHavePrefix;
	string runMode;

	if(argc==3){
		localfilePath=converCharPtoStr(argv[1]);
		runMode=converCharPtoStr(argv[2]);
		size_t lastBackslash=localfilePath.find_last_of('\\');
		size_t dotlast=localfilePath.find_last_of('.');

		// 获取要上传的文件夹名称
		foldername=string(localfilePath.begin()+string(localfilePath.begin(),localfilePath.begin()+lastBackslash).find_last_of('\\')+1,localfilePath.begin()+lastBackslash);
		// 获取当前的文件名称
		filename=string(localfilePath.begin()+lastBackslash+1,localfilePath.begin()+dotlast);
		// 带文件后缀的当前文件名
		filenameHavePrefix=string(localfilePath.begin()+lastBackslash+1,localfilePath.end());
		// 当前文件夹所在的绝对路径
		localNoFileNamePath=string(localfilePath.begin(),localfilePath.begin()+lastBackslash);
	}else{
		howuse();
		return 0;
	}
	// cout<<localfilePath<<endl;
	if(!checkHaveIniFile(binaryAbsPath+"\\setting.ini")){
		newIniFile(binaryAbsPath+"\\setting.ini");
	}else{
		setting.load(binaryAbsPath+"\\setting.ini");
	}

	// check ini file key and value
	if(!(checkIniKey("host")&&
		checkIniKey("port")&&
		(checkIniKey("password")||checkIniKey("sshKey"))&&
		checkIniKey("compiler")&&
		checkIniKey("optimizied")&&
		checkIniKey("stdver")&&
		checkIniKey("otherCompileArgs")&&
		checkIniKey("remoteTempFolder")&&
		checkIniKey("uploadTo"))
	)
	{
		cout<<"Please re-fill in the ini file and try again."<<endl;
		system(("C:\\Windows\\System32\\notepad "+binaryAbsPath+"\\setting.ini").c_str());
		return 0;
	}
	unionArgs["remoteTempFolder"]=getUplodeFullPath(unionArgs["remoteTempFolder"]);

	// matching language chose different compiler and standard version
	if(matchLangCompiler(getFilePrefix(converCharPtoStr(argv[1])))=="cpp"){
		switch(hashNum(unionArgs["compiler"])){
			case "clang"_HASH: {
				unionArgs["compiler"]+="++";
				break;
			}
			case "gcc"_HASH: {
				unionArgs["compiler"]="g++";
				break;
			}
			default:{
				cout<<"compiler error!please try clang/gcc."<<endl;
				break;
			}
		}
	}else{
			unionArgs["stdver"]="-std=c99";
			if(!(unionArgs["compiler"]=="clang"||unionArgs["compiler"]=="gcc")){
				cout<<"compiler error!please try clang/gcc."<<endl;
			}
	}

	bool sambaState=false;
	if(checkIniKey("sambaDrive")&&checkIniKey("remoteSambaPath")){
		sambaState=true;
	}
	// merge args
	string pscp;
	string plink;
	string sshlink;
	string cleanRemoteTemp;
	string checkRemoteTempFolder,createNowTimeFolder;
	string uploadAndRun;
	string commandArgs=mergeStr({unionArgs["compiler"],unionArgs["optimizied"],filename,filenameHavePrefix,unionArgs["stdver"],unionArgs["otherCompileArgs"]});
	string start="./"+filename;
	string sshclear="rm "+filename+"*";
	string servedVerifyMode;
	if(checkIniKey("password")){
		servedVerifyMode="-pw "+unionArgs["password"];
	}else{
		servedVerifyMode="-i ",unionArgs["sshKey"];
	}

	// merge command line
	string sshPlink=sshBinaryMerge("plink",servedVerifyMode);
	string sshPscp=sshBinaryMerge("pscp",servedVerifyMode);
	string sshPutty=sshBinaryMerge("putty",servedVerifyMode);
	{
		// 检查远程主机的临时目录是否存在
		checkRemoteTempFolder=mergeStr({"\""+sshPlink,"\"[ -d",unionArgs["remoteTempFolder"],"]","&&","echo The folder exists.Is about to begin execution.|","mkdir -p",unionArgs["remoteTempFolder"]+"\"\""});
		// 在远程主机的临时目录下创建当前时间点的文件夹
		createNowTimeFolder=mergeStr({"\""+sshPlink,"\"[ -d",unionArgs["uploadTo"]+"/"+nowTime,"]","&&","echo The folder exists.Is about to begin execution.||","mkdir -p",unionArgs["uploadTo"]+"/"+getNowTime()+"\"\""});
		// 上传文件夹并执行
		uploadAndRun=mergeStr({"\""+sshPlink,"\"cd "+unionArgs["remoteTempFolder"]+"/"+nowTime+"/"+foldername,"&&"+commandArgs+"&&"+start+"\"\""});
		// 上传文件/文件夹
		pscp=mergeStr({"\""+sshPscp,"\""+localfilePath+"\"",unionArgs["host"]+":"+unionArgs["remoteTempFolder"]+"\""});
		// 编译并执行
		plink=mergeStr({"\""+sshPlink,"\"cd "+unionArgs["remoteTempFolder"],"&&"+commandArgs+"&&"+start,"&&"+sshclear+"\"\""});
		// 打开一个SSH连接窗口
		sshlink=mergeStr({"\""+sshPutty+"\""});
		// 清理远程主机的临时目录
		cleanRemoteTemp=mergeStr({"\""+sshPlink,"\"cd "+unionArgs["remoteTempFolder"],"&&","rm -rf "+string(nowTime.begin(),nowTime.begin()+4)+"* *.cc *.c *.cpp .h .hpp"+"\"\""});
	}
	// cout<<checkRemoteTempFolder<<endl
	// 	<<createNowTimeFolder<<endl
	// 	<<pscp<<endl
	// 	<<plink<<endl
	// 	<<sshlink<<endl
	// 	<<cleanRemoteTemp<<endl;
	if(!(::setting.getStringValue("RemoteCompileSSHSetting", "remoteTempFolder")=="/tmp")){
		cout<<"Check whether there is "<<unionArgs["remoteTempFolder"]<<"..."<<endl;
		system(checkRemoteTempFolder.c_str());
	}
	if(!sambaState||(*localfilePath.begin()!=*unionArgs["sambaDrive"].begin())){
		// cout<<"Upload"<<endl;
		// choose run mode
		switch(hashNum(runMode)){
			case "panelRun"_HASH: {
				// cout<<"directRun"<<endl;
				system(pscp.c_str());
				system(plink.c_str());
				break;
			}
			case "terminalRun"_HASH: {
				// cout<<"terminalRun"<<endl;
				system(pscp.c_str());
				// cout<<endl<<"Run Resault:"<<endl;
				system(plink.c_str());
				break;
			}
			case "openTerminal"_HASH: {
				// cout<<sshlink<<endl;
				cout<<"Connecting to "<<unionArgs["host"]<<"..."<<endl;
				system(sshlink.c_str());
				break;
			}
			case "uploadThisFile"_HASH: {
				uploadOutMessage(createNowTimeFolder,nowTime);
				pscp=uploadTo(sshPscp,localfilePath,"file");
				cout<<pscp<<endl;
				// cout<<uploadAndRun<<endl;
				system(pscp.c_str());
				cout<<"Successfully upload "<<filenameHavePrefix<<" to "<<unionArgs["host"]<<":"+unionArgs["uploadTo"]+"/"+nowTime<<endl;
				break;
			}
			case "uploadCurrentFolderAndRun"_HASH: {
				uploadOutMessage(createNowTimeFolder,nowTime);
				pscp=uploadTo(sshPscp,localfilePath,"folder");
				// cout<<pscp<<endl;
				// cout<<uploadAndRun<<endl;
				system(pscp.c_str());
				cout<<"Successfully upload "<<localNoFileNamePath<<" to "<<unionArgs["host"]<<":"+unionArgs["uploadTo"]+"/"+nowTime<<endl;
				// cout<<endl<<"Run Resault:"<<endl;
				system(uploadAndRun.c_str());
				break;
			}
			case "uploadCurrentFolderAndTerminalRun"_HASH: {
				// uploadOutMessage(createNowTimeFolder,nowTime);
				system(createNowTimeFolder.c_str());
				pscp=uploadTo(sshPscp,localfilePath,"folder");
				// cout<<pscp<<endl;
				// cout<<uploadAndRun<<endl;
				system(pscp.c_str());
				// cout<<"Successfully upload "<<localNoFileNamePath<<" to "<<unionArgs["host"]<<":"+unionArgs["uploadTo"]+"/"+nowTime<<endl;
				cout<<endl<<"Run Resault:"<<endl;
				system(uploadAndRun.c_str());
				break;
			}
			case "cleanUpTemp"_HASH: {
				cout<<"clean up "<<unionArgs["host"]<<":"<<unionArgs["uploadTo"]<<"..."<<endl;
				system(cleanRemoteTemp.c_str());
				cout<<"Clean up the success."<<endl;
				break;
			}
			default: {
				cout<<"Parameter error!"<<endl;
			}
		}
	}else{
		// cout<<"direct"<<endl;
		string currentFileOnSambaPath=sambdaMapToLocalParse(localfilePath);
		string currentSambaPath=string(currentFileOnSambaPath.begin(),currentFileOnSambaPath.begin()+currentFileOnSambaPath.find_last_of("/")+1);
		// cout<<currentSambaPath<<endl;
		string panelRunPlink=mergeStr({"\""+sshPlink,"\"cd "+currentSambaPath,"&&"+commandArgs+"&&"+start,"\"\""});
		// cout<<panelRunPlink<<endl;
		switch(hashNum(runMode)){
			case "panelRun"_HASH: {
				system(panelRunPlink.c_str());
				break;
			}
			case "terminalRun"_HASH: {
				system(panelRunPlink.c_str());
				break;
			}
			case "openTerminal"_HASH: {
				// cout<<sshlink<<endl;
				cout<<"Connecting to "<<unionArgs["host"]<<"..."<<endl;
				system(sshlink.c_str());
				break;
			}
			case "uploadThisFile"_HASH: {
				cout<<"current file in samba scope.\nPlease direct panelRun/terminalRun Mode."<<endl;
				break;
			}
			case "uploadCurrentFolderAndRun"_HASH: {
				cout<<"current file in samba scope.\nPlease direct panelRun/terminalRun Mode."<<endl;
				break;
			}
			case "uploadCurrentFolderAndTerminalRun"_HASH: {
				cout<<"current file in samba scope.\nPlease direct panelRun/terminalRun Mode."<<endl;
				break;
			}
			case "cleanUpTemp"_HASH: {
				cout<<"clean up "<<unionArgs["host"]<<":"<<unionArgs["uploadTo"]<<"..."<<endl;
				system(cleanRemoteTemp.c_str());
				cout<<"Clean up the success."<<endl;
				break;
			}
			default: {
				cout<<"Parameter error!"<<endl;
			}
		}
	}
	return 0;
}