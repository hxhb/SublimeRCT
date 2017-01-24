#include "inifile/inifile.h"
#include "inifile/inifile.cpp"
#include <iostream>
#include <fstream>
#include <io.h>
#include <map>
#include <set>
#include <ctime>
#include <direct.h>
#include <windows.h>

using namespace inifile;

#define MAXPATH 512

static IniFile setting; // global .ini  setting file
static map<string,string> unionArgs; // args pack of .ini

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
			  <<";sshKeyPath(e.g:C:\\\\Users\\\\imzlp\\\\.ssh)\nsshKeyPath=\n\n;compiler(e.g:gcc/clang)\ncompiler=clang\n"<<endl
			  <<";e.g:-o/-o2\noptimizied=-o\n\n;std version e.g:c99/c11 or c++0x/c++11"<<endl
			  <<"stdver=-std=c++11\n\n;other compile args e.g:-pthread\notherCompileArgs=-pthread\n"<<endl
			  <<";remote host temp folder e.g:/tmp\n;It is recommended that you leave the default values(/tmp)\nremoteTempFolder=/tmp\n"<<endl
			  <<";Upload SourceFile to remote host path\n;auto create a name is localtime of folder\nuploadTo=/tmp"<<endl;
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

// merge pscp
string mergePscp(const string& binaryAbsPath,const string& path,const string& other=""){
	string pscp;
	if(checkIniKey("password")){
		pscp=mergeStr({binaryAbsPath+"\\SSHTools\\pscp",other,"-P",unionArgs["port"],"-pw",unionArgs["password"],"\""+path+"\"",unionArgs["host"]+":"+getUplodeFullPath(unionArgs["uploadTo"])});
	}else{
		pscp=mergeStr({binaryAbsPath+"\\SSHTools\\pscp",other,"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\""+path+"\"",unionArgs["host"]+":"+getUplodeFullPath(unionArgs["uploadTo"])});
	}
	return pscp;
}

void uploadTo(const string& binaryAbsPath,string &pscp,const string &createNowTimeFolder,const string& localfilePath,const string& nowTime,const string& UpMode){
	cout<<"Check whether there is "<<unionArgs["uploadTo"]<<"..."<<endl;
	cout<<"Create "<<unionArgs["uploadTo"]<<"/"<<nowTime<<"..."<<endl;
	system(createNowTimeFolder.c_str());
	cout<<"The folder "<<unionArgs["uploadTo"]<<"/"<<nowTime<<" was created successfully."<<endl;
	// system(checkRemoteTempFolder.c_str());
	switch(hashNum(UpMode)){
		case "file"_HASH: {
			pscp=mergePscp(binaryAbsPath,localfilePath);
			break;
		}
		case "folder"_HASH: {
			pscp=mergePscp(binaryAbsPath,localfilePath,"-r");
			break;
		}
		default: {
			cout<<"Parameter error!"<<endl;
		}
	}
	pscp+="/"+nowTime;
	// cout<<pscp<<endl;
	system(pscp.c_str());
}

void howuse(void){
	cout<<"Usage:\n\tTheProgram SourceFileFullPath RunMode"<<endl;
	cout<<"RunMode:\n\tpanelRun: Run on SublimeText Panel."<<endl
		<<"\tterminalRun: Run on a new window."<<endl
		<<"\tuploadThisFile: Upload current file to remoteHost."<<endl
		<<"\tuploadThisFolder: Upload current folder to remoteHost."<<endl
		<<"\topenTerminal: Open a SSH link to remoteHost."<<endl
		<<"\tcleanUpRemoteTemp: clean up remote host temp folder."<<endl
		<<"E.g.:"<<endl
		<<"\tcommandExec D:\\TEST\\A.cc terminalRun"<<endl;
}
int main(int argc, char const *argv[])
{
	SetConsoleTitle("RemoteCompile");
	string binaryAbsPath=getTheProgramAbsPath();
	// cout<<binaryAbsPath<<endl;
	string localfilePath,localNoFileNamePath,filename,filenameHavePrefix;
	string runMode;

	if(argc==3){
		localfilePath=converCharPtoStr(argv[1]);
		runMode=converCharPtoStr(argv[2]);
		size_t lastBackslash=localfilePath.find_last_of('\\');
		size_t dotlast=localfilePath.find_last_of('.');
		filename=string(localfilePath.begin()+lastBackslash+1,localfilePath.begin()+dotlast);
		filenameHavePrefix=string(localfilePath.begin()+lastBackslash+1,localfilePath.end());
		localNoFileNamePath=string(localfilePath.begin(),localfilePath.begin()+lastBackslash);
	}else{
		howuse();
		return 0;
	}

	if(!checkHaveIniFile(binaryAbsPath+"\\setting.ini")){
		newIniFile(binaryAbsPath+"\\setting.ini");
	}else{
		setting.load(binaryAbsPath+"\\setting.ini");
	}

	// check ini file key and value
	if(!(checkIniKey("host")&&
		   checkIniKey("port")&&
		   checkIniKey("compiler")&&
		   checkIniKey("optimizied")&&
		   checkIniKey("stdver")&&
		   checkIniKey("remoteTempFolder")&&
		   (checkIniKey("sshKeyPath")||checkIniKey("password"))&&
		   checkIniKey("otherCompileArgs")&&
		   checkIniKey("uploadTo")
	    )
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

	// merge args
	string pscp;
	string plink;
	string sshlink;
	string cleanRemoteTemp;
	string checkRemoteTempFolder,createNowTimeFolder;
	string nowTime=getNowTime();
	string commandArgs=mergeStr({unionArgs["compiler"],unionArgs["optimizied"],filename,filename+"*",unionArgs["stdver"],unionArgs["otherCompileArgs"]});
	string start="./"+filename;
	string sshclear="rm "+filename+"*";
	if(checkIniKey("password")){
		checkRemoteTempFolder=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],"-pw",unionArgs["password"],"\"[ -d",unionArgs["remoteTempFolder"],"]","&&","echo The folder exists.Is about to begin execution.||","mkdir -p",unionArgs["remoteTempFolder"],"\""});
		createNowTimeFolder=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],"-pw",unionArgs["password"],"\"[ -d",unionArgs["uploadTo"]+"/"+nowTime,"]","&&","echo The folder exists.Is about to begin execution.||","mkdir -p",unionArgs["uploadTo"]+"/"+getNowTime(),"\""});
		pscp=mergeStr({binaryAbsPath+"\\SSHTools\\pscp","-P",unionArgs["port"],"-pw",unionArgs["password"],"\""+localfilePath+"\"",unionArgs["host"]+":"+unionArgs["remoteTempFolder"]});
		plink=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],"-pw",unionArgs["password"],"\"cd "+unionArgs["remoteTempFolder"],"&&"+commandArgs+"&&"+start,"&&"+sshclear,"\""});
		sshlink=mergeStr({binaryAbsPath+"\\SSHTools\\putty","-P",unionArgs["port"],"-pw",unionArgs["password"],unionArgs["host"]});
		// string year=string(nowTime.begin()+nowTime.begin()+4);
		cleanRemoteTemp=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],"-pw",unionArgs["password"],"\"cd "+unionArgs["remoteTempFolder"],"&&","rm -rf "+string(nowTime.begin(),nowTime.begin()+4)+"* *.cc *.c *.cpp .h .hpp","\""});
	}else{
		checkRemoteTempFolder=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\"[ -d",unionArgs["remoteTempFolder"],"]","&&","echo The folder exists.Is about to begin execution.|","mkdir -p",unionArgs["remoteTempFolder"],"\""});
		createNowTimeFolder=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\"[ -d",unionArgs["uploadTo"]+"/"+nowTime,"]","&&","echo The folder exists.Is about to begin execution.||","mkdir -p",unionArgs["uploadTo"]+"/"+getNowTime(),"\""});
		pscp=mergeStr({binaryAbsPath+"\\SSHTools\\pscp","-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\""+localfilePath+"\"",unionArgs["host"]+":"+unionArgs["remoteTempFolder"]});
		plink=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\"cd "+unionArgs["remoteTempFolder"],"&&"+commandArgs+"&&"+start,"&&"+sshclear,"\""});
		sshlink=mergeStr({binaryAbsPath+"\\SSHTools\\putty","-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk",unionArgs["host"]});
		cleanRemoteTemp=mergeStr({binaryAbsPath+"\\SSHTools\\plink",unionArgs["host"],"-P",unionArgs["port"],unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\"cd "+unionArgs["remoteTempFolder"],"&&","rm -rf "+string(nowTime.begin(),nowTime.begin()+4)+"* *.cc *.c *.cpp .h .hpp","\""});
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
			uploadTo(binaryAbsPath,pscp,createNowTimeFolder,localfilePath,nowTime,"file");
			// string SourceFilePath=converCharPtoStr(argv[1]);
			cout<<"Successfully upload "<<filenameHavePrefix<<" to "<<unionArgs["host"]<<":"+unionArgs["uploadTo"]+"/"+nowTime<<endl;
			break;
		}
		case "uploadCurrentFolder"_HASH: {
			uploadTo(binaryAbsPath,pscp,createNowTimeFolder,localNoFileNamePath,nowTime,"folder");
			cout<<"Successfully upload "<<localNoFileNamePath<<" to "<<unionArgs["host"]<<":"+unionArgs["uploadTo"]+"/"+nowTime<<endl;
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
	return 0;
}