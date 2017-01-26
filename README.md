### Update:2017.01.26
1. 优化代码。
2. 增加另一种机制：如果将远程主机的目录通过samba映射到本地，对其中的文件夹进行编译时直接对该远程目录的源文件进行操作(将本地映射路径转换为远程主机的绝对路径)，不执行上传源文件这一步骤(因为该文件已经存在于远程主机了)。
比如：远程主机有一个samba文件夹`~/code`，我将其映射到了我的Z盘，我在Z盘(也就是远程主机的`~/code`)下新建源文件`hw.cc`，然后使用本插件进行远程编译时，会直接执行编译操作(非samba下会先上传再编译)，从而提高运行速度。

使用方式：
需要在`setting.ini`指定`sambaDrive`项为远程samba映射到本地的盘符，`remoteSambaPath`为映射到本地`sambaDrive`盘符的远程主机samba文件夹路径。

```ini
;远程主机的~/目录被映射到本地的Z盘

;samba mapping to local DeskDrive
sambaDrive=Z

;remote sambda Path
remoteSambaPath=~/
```

这样，在本地的`Z盘`中的文件执行“远程编译”不会被执行上传(`UploadThisFile`和`UploadCurrentFolder`模式均不会执行)。

---------

经常在Win上写一些跑在Linux上的测试小代码还需要手动在Linux下执行编译命令有些麻烦，而且我用树莓派配置samba将代码共享到Win上也需要ssh上去手动编译，有点浪费时间。

这几天闲时写了一个SublimeText的小插件，用来在windows下远程编译C/C++的代码，就是在Windows上写代码但是实际会在Linux上执行。目前只是实现了功能，等放假后休息时有空优化一下。

代码放在Github上：[sublimeRemoteCompile](https://github.com/hxhb/sublimeRemoteCompile)，使用了一些`C++11`的特性，编译时需指定。

裹了一层ssh操作，用来写一些简单的测试代码还是挺爽的，错误信息也会捕获到SublimeText的Panel中。复杂项目可以使用VisualStudio+VisualGDB。
通过ini读取配置信息，具有六种运行模式：

| 参数                  | 模式                        |
| :-----------------: | :-----------------------: |
| panelRun            | 运行在SublimeText的Panel      |
| terminalRun         | 在新窗口中运行                   |
| uploadThisFile      | 上传当前打开的文件到远程主机的临时目录       |
| uploadCurrentFolder | 上传当前打开的文件所在的文件夹到远程主机的临时目录 |
| cleanUpTemp         | 清理远程主机的临时目录               |
| openTerminal        | 打开一个SSH连接窗口到远程主机          |

运行模式需要在启动时指定。
运行时需要指定两个参数：源文件路径和运行模式。在SublimeText的`build system`中源文件路径可以通过`${file}`获得。

编译时会根据源文件后缀判断语言，C语言默认使用gcc编译，C++则会使用g++，对应的标准版本则是c99和c++11。建议在Linux环境上安装clang，然后在`setting.ini`中指定编译器为`clang`，错误提示好很多。如果指定`clang`也会自动匹配后缀选择编译器为`clang`或者`clang++`。
#### 如何使用？

在[这里](https://github.com/hxhb/sublimeRemoteCompile/releases/tag/v1.0)下载二进制压缩包，解压到本地的一个目录中。

然后在SublimeText中添加一个`build system`，然后将下面的代码填入其中：

```json
{
    // binary absolute path
    "path": "C:\\Program Files (x86)\\SystemTools\\Tookit\\RemoteCompile",
    "cmd": ["cmd", "/c","remotecompile","${file}","panelRun"],
    "file_regex": "^(..[^:]*):([0-9]+):?([0-9]+)?:? (.*)$",
     "selector": "source.c,source.cc,source.cpp,source.c++",
     "shell": true,
     "variants":
     [
          {
                "name": "TerminalRun",
                 "cmd": ["start", "cmd", "/k","remotecompile","${file}","terminalRun"]
          },
          {
                "name": "UploadThisFile",
                 "cmd": ["cmd", "/c","remotecompile","${file}","uploadThisFile"]
          },
          {
                "name": "UploadCurrentFolder",
                 "cmd": ["cmd", "/c","remotecompile","${file}","uploadCurrentFolder"]
          },
          {
                "name": "CleanUpTemp",
                "cmd": ["cmd", "/k","remotecompile","${file}","cleanUpTemp"]
          },
          {
                "name": "OpenTerminal",
                 "cmd": ["cmd", "/k","remotecompile","${file}","openTerminal"]
          }
     ]
}
```

在`path`处填上你将压缩包解压到的位置。
然后可以简单写个代码，按`Ctrl+Shift+B`就可以看到构建系统列表了：

![00](http://7xilo9.com1.z0.glb.clouddn.com/blog-images/sublimeTextRemoteCompilePlugins/00.png)

随便选择一个，会开始执行。
如果是首次使用，会在二进制所在目录生成一个`setting.ini`文件，需要填写你自己的环境信息：

```ini
[RemoteCompileSSHSetting]
;remote host addr(e.g root@192.168.137.2)
host=
;remote host port(e.g:22)
port=22
;remote host user password
password=
;sshKey(e.g:C:\\Users\\imzlp\\.ssh\\id_rsa.ppk)
sshKey=
;compiler(e.g:gcc/clang)
compiler=gcc
;e.g:-o/-o2
optimizied=-o
;std version e.g:c99/c11 or c++0x/c++11
stdver=-std=c++11
;other compile args e.g:-pthread
otherCompileArgs=-pthread
;remote host temp folder e.g:/tmp
;It is recommended that you leave the default values(/tmp)
remoteTempFolder=/tmp
;Upload SourceFile to remote host path
;auto create a name is localtime of folder
uploadTo=/tmp
```

建议填写`host`和`password`或者`sshKey`，其他保持默认即可。
另外，`sshKey`要求是ppk秘钥，需要将ssh的秘钥使用`puttygen`转换成ppk才可以(`puttygen`在SSHTools目录下)，建议使用`password`。

### panelRun
![01](http://7xilo9.com1.z0.glb.clouddn.com/blog-images/sublimeTextRemoteCompilePlugins/01.png)
### terminalRun
![02](http://7xilo9.com1.z0.glb.clouddn.com/blog-images/sublimeTextRemoteCompilePlugins/02.png)
### uploadThisFile
![03](http://7xilo9.com1.z0.glb.clouddn.com/blog-images/sublimeTextRemoteCompilePlugins/03.png)
### uploadCurrentFolder
![04](http://7xilo9.com1.z0.glb.clouddn.com/blog-images/sublimeTextRemoteCompilePlugins/04.png)
### cleanUpTemp
![05](http://7xilo9.com1.z0.glb.clouddn.com/blog-images/sublimeTextRemoteCompilePlugins/05.png)
### openTerminal
![06](http://7xilo9.com1.z0.glb.clouddn.com/blog-images/sublimeTextRemoteCompilePlugins/06.png)
