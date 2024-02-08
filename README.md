<h1 align='center'>幻兽帕鲁单机存档转专用服务器修复工具</h1>

<p align="center">
   <strong>简体中文</strong> | <a href="/README.en.md">English</a>
</p>
<p align='center'>
  通过修复SteamID Hash计算和设置原始单机SteamID来让服务器无需修改存档文件即可无缝使用单机存档<br/>
</p>

<p align='center'>
<img alt="GitHub Repo stars" src="https://img.shields.io/github/stars/number201724/PalServerHostFix?style=for-the-badge">&nbsp;&nbsp;
</p>

## 安装部署

1. 使用SteamCMD更新PalServer

   ```cmd
   steamcmd.exe +force_install_dir C:\PalServer +login anonymous +app_update 2394010 validate +quit
   ```

2. 下载单机存档修复工具并解压到PalServer目录 

3. 复制本地PalWorld的存档到服务器的存档目录下，并且记住SteamId和Hash，等下要用

   ``` cmd
   %AppData%\..\Local\Pal\Saved\SaveGames\{SteamId}\{Hash}
   ```

   ```
   C:\PalServer\Pal\Saved\SaveGames\0\
   ```

4. 删除存档中的WorldOption.sav，否则将不能使用PalWorldSettings.ini修改服务器配置

5. 修改PalServer\Pal\Saved\Config\WindowsServer\GameUserSettings.ini的如下配置以让存档正确使用地图

   ```cmd
   [/Script/Pal.PalGameLocalSettings]
   DedicatedServerName={Hash}
   ```

   

6. 修改PalHost.ini中的SteamId为上面的SteamId，以让PalServer正确识别单机存档下的主机SteamId

7. 使用PalServerHost.exe启动服务器，当PalServer出现下面的提示表示启动成功

   ```
   Setting breakpad minidump AppID = 1623730
   ```

## 许可证

根据 [Apache2.0 许可证](LICENSE) 授权，任何转载请在 README 和文件部分标明！任何商用行为请务必告知！
