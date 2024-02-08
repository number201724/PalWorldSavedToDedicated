<h1 align='center'>幻兽帕鲁单机存档转专用服务器修复工具</h1>

<p align="center">
   <a href="/README.md">简体中文</a> | <strong>English</strong>
</p>
<p align='center'>
  By fixing the SteamID Hash calculation and setting the original single-player SteamID, the server can seamlessly use single-player saves without modifying the save file.<br/>
</p>


<p align='center'>
<img alt="GitHub Repo stars" src="https://img.shields.io/github/stars/number201724/PalServerHostFix?style=for-the-badge">&nbsp;&nbsp;
</p>

## Installation

1. Update PalServer using SteamCMD

   ```cmd
   steamcmd.exe +force_install_dir C:\PalServer +login anonymous +app_update 2394010 validate +quit
   ```

2. Download the dedicated server fix tool and extract it to the PalServer directory

3. Copy the local PalWorld saved to the server's saved directory, and remember the SteamId and Hash

   ``` cmd
   %AppData%\..\Local\Pal\Saved\SaveGames\{SteamId}\{Hash}
   ```

   ```
   C:\PalServer\Pal\Saved\SaveGames\0\
   ```

4. Delete WorldOption.sav, otherwise you will not be able to use PalWorldSettings.ini to modify the server configuration

5. Modify the following configuration of PalServer\Pal\Saved\Config\WindowsServer\GameUserSettings.ini to allow the saved to use the map correctly

   ```cmd
   [/Script/Pal.PalGameLocalSettings]
   DedicatedServerName={Hash}
   ```

   

6. Modify the SteamId in PalHost.ini to the SteamId above to allow PalServer to correctly identify the host SteamId under the single-player saved.

7. Use PalServerHost.exe to start the server. When PalServer displays the following prompt, it means the startup is successful.

   ```
   Setting breakpad minidump AppID = 1623730
   ```

## License

Authorized under [Apache2.0 License](LICENSE), please indicate any reprint in the README and file sections! Please be sure to inform us of any commercial activities!
