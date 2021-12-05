# WindowsDesktopApps

## PrivateMesanger

A Window x64 text messaging appliction. 

Designed to work with the companion AlgoMachines CGI component as a server. The current version of the CGI  component supports up to 10,000 clients.

### Installation and use

1) Simply download PrivateMessanger.exe to an empty directory on your PC
2) Launch the .exe
3) Enter an application password when prompted
4) Enter the URL of the server which has the PrivateMessanger CGI component. The AlgoMachines server for this purpose has URL: 23.254.165.221
5) During installation you client generates a random 32 byte id, see it in the Help-About dialog
6) Initate a new conversation with another client: App-Initiate New Conversation, note that the AlgoMachines client has ID: 1yXRvGr5rbhiOntxopSYjYQD1fkDemY5GSsGyvyATG9

### Privacy

Your messages are encrypted with your app password as they reside on your PC. Messages on the server are deleted as soon as they are downloaded to the recipient client.

Privacy in transit is guaranteed by AlgoMachines' novel protected code security mechanism.

https://github.com/algomachines/WindowsDesktopApps/blob/main/DOC/PrivateMessanger_SecurityModel.png

