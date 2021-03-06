# WindowsDesktopApps

## PrivateMessenger

A Window x64 text messaging application. 

Designed to work with the companion AlgoMachines CGI component as a server. The current version of the CGI  component supports up to 10,000 clients.

### Motivation / Why PrivateMessenger

The only way to achieve real privacy and personal sovereignty over our data is for people to take the server side into their own hands. To that end, we will open the source of the PrivateMessenger CGI component.

This is a complete client/server solution for text messaging which requires no configuration or tuning, no IT expertise, no third parties. You will be able to set up your own text message server immediately if you have a Windows http server.

### Installation and use

1) Simply download PrivateMessanger.exe to an empty directory on your PC
2) Launch the .exe
3) Enter an application password when prompted
4) Enter the URL of the server which has the PrivateMessanger CGI component. The AlgoMachines server for this purpose has URL: 23.254.165.221
5) During installation your client generates a random 32 byte id, see it in the Help-About dialog
6) Initate a new conversation with another client: App-Initiate New Conversation, note that the AlgoMachines client has ID: 1yXRvGr5rbhiOntxopSYjYQD1fkDemY5GSsGyvyATG9

Up to 20 pending messages (not retrieved by the recipient yet) may be made by a client. Beyond that, older pending messages are deleted by the server before new ones are added. This saves space and prevents spamming.

Pending messages become stale after 7 days and are deleted by the server if they are not downloaded by the recipient client.

A total of no more than 3000 pending messages may reside on a single server.

### Privacy

Your messages are encrypted with your app password as they reside on your PC. Messages on the server are deleted as soon as they are downloaded to the recipient client.

Privacy in transit is guaranteed by AlgoMachines' novel protected code security mechanism.

https://github.com/algomachines/WindowsDesktopApps/blob/main/DOC/PrivateMessenger_SecurityModel.png

### Download PrivateMessenger Client

https://github.com/algomachines/WindowsDesktopApps/raw/main/BIN/PrivateMessenger.exe

### About the source code

The complete source code for the Windows client is included in this repository. This has been developed using Visual Studio 2019.
