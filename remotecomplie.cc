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

static IniFile setting; // global ini file
static map<string,string> unionArgs; // args of .ini
// static string iniFilePath; // fullpath of .ini file

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
	if(!access(execPath.c_str(),F_OK)){
		// printf("have\n");
		return true;
	}else{
		// printf("no\n");
		return false;
	}
}

// if .ini is not found,create a .ini file
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
string getExecPath(void){
	char buffer[MAXPATH];
	getcwd(buffer, MAXPATH);
	string execPath=converCharPtoStr(buffer);
	return execPath;
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

string mergePscp(const string& path,const string& other=""){
	string pscp;
	if(checkIniKey("password")){
		pscp=mergeStr({"pscp",other,"-P",unionArgs["port"],"-pw",unionArgs["password"],"\""+path+"\"",unionArgs["host"]+":"+getUplodeFullPath(unionArgs["uploadTo"])});
	}else{
		pscp=mergeStr({"pscp",other,"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\""+path+"\"",unionArgs["host"]+":"+getUplodeFullPath(unionArgs["uploadTo"])});
	}
	return pscp;
}

void uploadTo(string &pscp,const string &createNowTimeFolder,const string& localfilePath,const string& nowTime){
	cout<<"Check whether there is "<<unionArgs["uploadTo"]<<"..."<<endl;
	cout<<"Create "<<unionArgs["uploadTo"]<<"/"<<nowTime<<"..."<<endl;
	system(createNowTimeFolder.c_str());
	// system(checkRemoteTempFolder.c_str());
	pscp=mergePscp(localfilePath);
	pscp+="/"+nowTime;
	// cout<<pscp<<endl;
	system(pscp.c_str());
}

int main(int argc, char const *argv[])
{
	SetConsoleTitle("RemoteCompile");
	string localfilePath,localNoFileNamePath,filename;
	string runStyle;

	if(argc==3){
		localfilePath=converCharPtoStr(argv[1]);
		// iniFilePath=converCharPtoStr(argv[2]);
		auto lastBackslash=localfilePath.find_last_of('\\')+1;
		auto dotlast=localfilePath.find_last_of('.');
		filename=string(localfilePath.begin()+lastBackslash,localfilePath.begin()+dotlast);
		localNoFileNamePath=string(localfilePath.begin(),localfilePath.begin()+lastBackslash);
		fixWinPathDoubleSprit(localNoFileNamePath);
		runStyle=converCharPtoStr(argv[2]);
	}else{
		cout<<"Usage:\n\tTheExec SourceFileFullPath RunStyle"<<endl;
		cout<<"RunStyle:\n\tpanelRun\tterminalRun\tuploadThisFile\tuploadThisFolder\topenTerminal"<<endl;
		cout<<"E.g.:"<<endl;
		cout<<"\tcommandExec D:\\\\TEST\\\\A.cc terminalRun"<<endl;
		return 0;
	}

	if(!checkHaveIniFile("setting.ini")){
		newIniFile("setting.ini");
	}else{
		setting.load("setting.ini");
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
		system(("notepad "+getExecPath()+"\\setting.ini").c_str());
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
	string checkRemoteTempFolder,createNowTimeFolder;
	string nowTime=getNowTime();
	string commandArgs=mergeStr({unionArgs["compiler"],unionArgs["optimizied"],filename,filename+"*",unionArgs["stdver"],unionArgs["otherCompileArgs"]});
	string start="./"+filename;
	string sshclear="rm "+filename+"*";

	if(checkIniKey("sshKeyPath")){
		checkRemoteTempFolder=mergeStr({"plink",unionArgs["host"],"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\"[ -d",unionArgs["remoteTempFolder"],"]","&&","echo The folder exists.Is about to begin execution.|","mkdir -p",unionArgs["remoteTempFolder"],"\""});
		createNowTimeFolder=mergeStr({"plink",unionArgs["host"],"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\"[ -d",unionArgs["uploadTo"]+"/"+nowTime,"]","&&","echo The folder exists.Is about to begin execution.||","mkdir -p",unionArgs["uploadTo"]+"/"+getNowTime(),"\""});
		pscp=mergeStr({"pscp","-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\""+localfilePath+"\"",unionArgs["host"]+":"+unionArgs["remoteTempFolder"]});
		plink=mergeStr({"plink",unionArgs["host"],"-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk","\"cd "+unionArgs["remoteTempFolder"],"&&"+commandArgs+"&&"+start,"&&"+sshclear,"\""});
		sshlink=mergeStr({"putty","-P",unionArgs["port"],"-i",unionArgs["sshKeyPath"]+"\\\\id_rsa.ppk",unionArgs["host"]});
	}else{
		checkRemoteTempFolder=mergeStr({"plink",unionArgs["host"],"-P",unionArgs["port"],"-pw",unionArgs["password"],"\"[ -d",unionArgs["remoteTempFolder"],"]","&&","echo The folder exists.Is about to begin execution.||","mkdir -p",unionArgs["remoteTempFolder"],"\""});
		createNowTimeFolder=mergeStr({"plink",unionArgs["host"],"-P",unionArgs["port"],"-pw",unionArgs["password"],"\"[ -d",unionArgs["uploadTo"]+"/"+nowTime,"]","&&","echo The folder exists.Is about to begin execution.||","mkdir -p",unionArgs["uploadTo"]+"/"+getNowTime(),"\""});
		pscp=mergeStr({"pscp","-P",unionArgs["port"],"-pw",unionArgs["password"],"\""+localfilePath+"\"",unionArgs["host"]+":"+unionArgs["remoteTempFolder"]});
		plink=mergeStr({"plink",unionArgs["host"],"-P",unionArgs["port"],"-pw",unionArgs["password"],"\"cd "+unionArgs["remoteTempFolder"],"&&"+commandArgs+"&&"+start,"&&"+sshclear,"\""});
		sshlink=mergeStr({"putty","-P",unionArgs["port"],"-pw",unionArgs["password"],unionArgs["host"]});
	}

	if(!(::setting.getStringValue("RemoteCompileSSHSetting", "remoteTempFolder")=="/tmp")){
		cout<<"Check whether there is "<<unionArgs["remoteTempFolder"]<<"..."<<endl;
		system(checkRemoteTempFolder.c_str());
	}

	// call ssh
	switch(hashNum(runStyle)){
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
			uploadTo(pscp,createNowTimeFolder,localfilePath,nowTime);
			string SourceFilePath=converCharPtoStr(argv[1]);
			cout<<"Successfully upload "<<filename+"."+string(SourceFilePath.begin()+SourceFilePath.find_last_of('.')+1,SourceFilePath.end())<<" to "<<unionArgs["host"]<<":"+unionArgs["uploadTo"]<<endl;
			break;
		}
		case "uploadThisFolder"_HASH: {
			uploadTo(pscp,createNowTimeFolder,localfilePath,nowTime);
			cout<<"Successfully upload "<<localNoFileNamePath<<" to "<<unionArgs["host"]<<":"+unionArgs["uploadTo"]+"/"+nowTime<<endl;
			break;
		}
		default: {
			cout<<"Parameter error!"<<endl;
		}
	}
	return 0;
}