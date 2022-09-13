/* 參考資料網站： http://zake7749.github.io/2015/03/17/SocketProgramming/ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream> //string使用
#include <fstream>	//MyReadFile、getline使用
#include <map>
#include <string>
//using namespace std;//不能用這個，只能每一個函式自己掛上std::。//親身慘痛經歷
#include <netdb.h>		 // getaddrinfo()使用
#include <arpa/inet.h> //inet_ntop()使用
#include <vector>
#include <list>

enum ClientState
{
	IDLE,
	DNS,
	TABLE
};

int main(int argc, char *argv[])
{
	/*----------------------------------------------------------*/
	// 處理助教所給的txt檔案
	std::ifstream MyReadFile("query.txt");

	std::map<std::string, std::string> Map;
	std::string line;
	while (getline(MyReadFile, line))
	{
		std::string ID, Email;
		bool ID_OKed = false;
		for (const auto &c : line)
		{
			if (isdigit(c) && !ID_OKed)
				ID += c;
			else if (isblank(c))
				ID_OKed = true;
			else
				Email += c;
		}

		Map.insert(std::pair<std::string, std::string>(ID, Email));
	}

	MyReadFile.close();
	std::cout << "\nServerSelfStatus: Table constructed successfully with " << Map.size() << " entries.\n"
						<< std::endl;
	// 處理助教所給的txt檔案
	/*----------------------------------------------------------*/

	/*----------------------------------------------------------*/
	//本地端（a.k.a. Server端）的socket的建立
	char inputBuffer[256] = {};																																																									//接收來自Client的訊息用。每次接收Client的訊息，都會覆蓋前一次的內容！
	int sockfd;																																																																	/*註冊Server本地的Socket之後，會收到的的描述符（描述符是啥，請見本code上方的參考資料網站，然後去查詢維基百科。其實就是類似open()所回傳的檔案標頭）*/
	sockfd = socket(AF_INET /*NET指涉是使用IPv4，而不是IPv6，也不是單純只能LocalHost執行，而是可以真正www傳遞*/, SOCK_STREAM /*指定用TCP*/, 0); //基本上這是固定用法，沒什麼好改動的
	//本地端（a.k.a. Server端）的Socket創建如果成功，sockfd這個描述符就不會是-1
	if (sockfd == -1)
	{
		printf("ServerSelfStatus: Fail to create a socket.\n");
		return 0;
	}
	else
		std::cout << "ServerSelfStatus: Socket Created!\n";
	std::cout << "ServerSelfStatus: SockFD is: " << sockfd << std::endl
						<< std::endl;
	//本地端（a.k.a. Server端）的socket的建立
	//！！目前只是指定了socket是TCP格式！！目前我們還沒有對目前這個Server端的Socket，所願意接受的IPAddr、socket指定要用的port進行設定！
	/*----------------------------------------------------------*/

	/*----------------------------------------------------------*/
	//socket的真正細部設定
	struct sockaddr_in serverInfo;
	bzero(&serverInfo, sizeof(serverInfo)); //把serverInfo這個Struct的「初始位置」開始的sizeof(serverInfo)這麼多bit（bzero的b），全部設定成0。有做出事清空，有保庇。

	serverInfo.sin_family = PF_INET;				 //AF_INET,因為這是IPv4。//PF_INET就是AF_INET，這是在define裡面寫好的。
	serverInfo.sin_addr.s_addr = INADDR_ANY; //指定這個Server端，的白名單是所有的IPAddr。不寫INADDR_ANY也可以，你就要輸入數字，類似140.114.28.37之類的，但是要把這串依據網路的習慣（好像是BigEndian）換成唯一的整數，就需要調用到inet_addr("123.123.13.12");函數。此函數雖然有人說有定義在<sys/socket.h>，但實測我沒辦法找到，需要額外include <arpa/inet.h>才可以。
	//接續上一行。因為助教可能從www上面去對我的Server連線，所以我不能設定成127.0.0.1！
	serverInfo.sin_port = htons(1234); //指定這個Server要在1234這個port上面運行。
	//socket的真正細部設定
	/*----------------------------------------------------------*/

	/*----------------------------------------------------------*/
	//上面的第一段，是在初始化Socket，並且獲得SocketFD這個代號。
	//上面的第二段，是在設定serverInfo，這個是用來設定Socket的細部內容。
	//但是我們還沒有把SocketFD和擁有細部內容的那個struct綁定在一起！
	bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
	//綁定完成！現在Socket已經可以運作了
	//PS：只有server才需要做綁定。如果是Client端，一樣會有這邊所寫的serverInfo（當然變數名稱不會是這個），但是它就不需要綁定，因為這個東東就直接餵給connect()就好了！
	/*----------------------------------------------------------*/

	/*----------------------------------------------------------*/
	listen(sockfd, 5);
	//當港口出入頻繁時，我們得讓來客照拜訪的先後排成隊列，即是說每當一個請求送到Server，Socket就會把它丟到監聽隊列的尾端。
	//把這個隊列的大小，設定成5。也就是，規定最多能有幾個人能同時連入server（但當然還是分批、分時間服務）。
	/*----------------------------------------------------------*/

	/*----------------------------------------------------------*/
	//現在這裡就是負責和Client互通有無。當Client是誰呢？要等願意跟我連線的Client自己找上門。
	char message_to_welcome_client[] = {"Hi! Ready for service!\n\n"};

	int forClientSockfd /*Server會有自己的SocketFD(在本code的上端已經寫了)，Client也會有自己的SocketFD（在Client那邊的code）。而Server和Client兩端一旦建立好連線，也會有一個這個連線專屬用的SockFD。你完全可以想成計網概教的兩端各自擁有的private key和共有的session key*/ = 0;
	struct sockaddr_in clientInfo;
	int addrlen = sizeof(clientInfo);
	while (true)
	{
		//只要Server.exe一打開，就會一直在這個迴圈裡面繞。其實不應該說繞，因為根本沒有繞。如果沒有client找上這個Server，這個while裡面的accept就完全不會return任何東西，所以會卡在accept。
		forClientSockfd = accept(sockfd, (struct sockaddr *)&clientInfo, (socklen_t * /*如果不加上這個括弧，會跳出「int*和socklen_t*不相容」的錯誤*/) & addrlen);
		//上一行白話文： 我現在這個Server的Socket的描述符sockfd，願意跟Client連線！現在有誰想跟我連線呢？你願意？好的！你願意，所以你會把Client.exe裡面所儲存的你的clientInfo給我，這樣我就可以配發專屬這個溝通的session key(a.k.a. session SockFD)了！耶！
		//以上是Client連進來的時候的初始設定
		std::cout << "ServerSelfStatus: Connected to a client!" << std::endl
							<< std::endl;

		ClientState state = IDLE; //每一步都記錄Client的狀態，這樣如果他明明上次傳了一個2的訊息（a.k.a. 告訴我要Table），結果現在卻給我IPAdd人，這樣不合理。有了state，才有辦法報錯

		/*
		！！！Server和Client之間真正的訊息傳遞是用這邊的send()和recv()。還有其他函式可以用，可以自己上網查。我是把這兩個函式放在這裡當作模板使用。
		//以下是範例code的寫法
		//send(forClientSockfd, message_to_welcome_client, sizeof(message_to_welcome_client), 0);
		//recv(forClientSockfd, inputBuffer, sizeof(inputBuffer), 0);
		//printf("Get:%s\n", inputBuffer);

		//以下是我嘗試數百遍得到的正確不會有錯誤的寫法//親身慘痛經歷
		//注意！第一行一定是send，然後一定要有cout輸出（否則就是會沒反應，我也不懂），然後才是recv，recv完了之後才可以printf。這是個函式的指令順序不能改變！！！！！！
		send(forClientSockfd, message_to_welcome_client, sizeof(message_to_welcome_client), 0);
		std::cout << "ServerSelfStatus: Welcome message sent" << std::endl;
		recv(forClientSockfd, inputBuffer, sizeof(inputBuffer), 0);///////////
		printf("Client says: %s\n", inputBuffer);


		//還有一點要注意，Server和Client之間是你來我往，Server傳一個訊息給Client，Client可以顯示。但是，如果Client沒有回傳一個訊息給Server，在這個情況下，如果Server一直一直一直傳了100則訊息，Client也收不到。//親身慘痛經歷
		*/

		send(forClientSockfd, message_to_welcome_client, sizeof(message_to_welcome_client), 0);
		std::cout << "ServerSelfStatus: Welcome message sent." << std::endl;
		recv(forClientSockfd, inputBuffer, sizeof(inputBuffer), 0);
		//printf("Client says: %s\n", inputBuffer);//Client回傳： Nice to meet you, Server!

		char ask_what_to_do[] = {"What's your requirement? 1.DNS 2.QUERY 3.QUIT : "};
		send(forClientSockfd, ask_what_to_do, sizeof(ask_what_to_do), 0);
		std::cout << "ServerSelfStatus: Question sent." << std::endl;
		recv(forClientSockfd, inputBuffer, sizeof(inputBuffer), 0);
		//printf("Client says: %s\n", inputBuffer);//Client回傳： 有可能是各種各種下面那一段備註所列的其中一項，所以進入while迴圈去進行處理
		//上面兩行固定的格式，被放在while迴圈的開頭

		//使用者那邊傳過來的字串總共有幾類：
		//一串網址， 需要DNS去給出IPAddr
		//選項1～3（格式是： "x", "/", "\0"），其中3是需要單獨處理的（因為是Client要離開）
		//'?'，代表Server這邊不需理會
		//'!'，代表Client使用者輸入錯誤，所以要告訴他他有錯
		//一個大數字，代表的是學號，需要去查表給出Email

		while (true)
		{
			std::string message_to_reply;

			if (inputBuffer[0] == '3' && inputBuffer[1] == '/' && inputBuffer[2] == '\0')
			{
				state = IDLE;
				break;
			}
			//‘3’的Client離開，已經處理完畢

			else if (inputBuffer[0] == '2' && inputBuffer[1] == '/' && inputBuffer[2] == '\0' && state == IDLE)
			{
				state = TABLE;
				message_to_reply = "Input student ID : ";
			} //'2'已經處理完畢

			else if (inputBuffer[0] == '1' && inputBuffer[1] == '/' && inputBuffer[2] == '\0' && state == IDLE)
			{
				state = DNS;
				message_to_reply = "Input URL address : ";
			} //'1'已經處理完畢

			else if (inputBuffer[0] == '?')
				message_to_reply = "What's your requirement? 1.DNS 2.QUERY 3.QUIT : ";
			//'?'已經處理完畢

			else if (inputBuffer[0] == '!')
			{
				message_to_reply = "You typed sth wrong.\nThe # of service is 1-digit, but is in the range of 1 to 3 (inclusive).\nPlz try again.\n\n";
			} //'!'已經處理完畢

			//剩下「一串大數字」和「IPAddr」兩種可能
			//先處理一串大數字
			else if (isdigit(inputBuffer[0]))
			{
				//會來到這個else if，代表使用者輸入的是一串數字，是學號。那如果他上一個跟我講的是他要找IP，那就要跟他說這個訊息我看不懂。
				if (state == DNS)
				{
					message_to_reply = "You requested to do DNS, but now you give me the student ID. Access Denied! Server back to IDLE state.\n\n";

					state = IDLE;
				}
				else if (state == IDLE)
				{
					message_to_reply = "The server can't offer what you want with the given index. Access Denied!\n\n"; //原本是寫「"We" can't offer」，但因為Server那邊是用W和I去做分流，所以“We”不行！所以才改成這樣。下面也有一行是同個原因
				}
				else
				{
					//先把inputBuffer所有數字湊起來變成string，才能查表
					std::string num;
					for (auto i = 0; i < sizeof(inputBuffer) && inputBuffer[i] != '\0'; i++)
						num += inputBuffer[i];

					//有了使用者輸入的大數字以後，就可以查表了
					//注意，使用者可能輸入表中所沒有的數字
					auto it = Map.find(num);
					if (it != Map.end())
					{
						message_to_reply = "Email get from server : " + (*it).second + "\n\n";
					}
					else
					{
						message_to_reply = "Email get from server : No such student ID\n\n"; //此訊息內容是助教要求
					}

					state = IDLE;
					std::cout << "ServerSelfStatus: Request TableLookUp from user Complete." << std::endl;
				}
			}
			else
			{ //DNS的部分
				if (state == TABLE)
				{
					message_to_reply = "You requested to do TableLookUp, but now you give me sth that's not number. Access Denied! Server back to IDLE state.\n\n";

					state = IDLE;
				}
				else if (state == IDLE)
				{
					message_to_reply = "The server can't offer what you want with the given index. Access Denied!\n\n"; //原本是寫「"We" can't offer」，但因為Server那邊是用W和I去做分流，所以“We”不行！所以才改成這樣
				}
				else
				{ //真正的DNS的部分
					std::string URL(inputBuffer);

					struct addrinfo hints;
					struct addrinfo *receivedDNSinfo; // 將指向結果

					memset(&hints, 0, sizeof(hints)); // 確保 struct 為空
					hints.ai_family = AF_UNSPEC;			// 不用管是 IPv4 或 IPv6
					hints.ai_socktype = SOCK_STREAM;	// TCP stream sockets
					hints.ai_flags = AI_PASSIVE;			// 幫我填好我的 IP

					int getaddrinfo_sucOrFail = getaddrinfo(URL.c_str(), "80", &hints, &receivedDNSinfo);
					// servinfo 目前指向一個或多個 struct addrinfos 的鏈結串列

					if (getaddrinfo_sucOrFail != 0)
						message_to_reply = "Such DNS IP not found\n\n";
					else
					{
						char ipstr[INET6_ADDRSTRLEN];
						for (auto p = receivedDNSinfo; p != NULL; p = p->ai_next)
						{
							void *addr;
							char const *ipver;

							// 取得本身位址的指標，
							if (p->ai_family == AF_INET)
							{ // IPv4
								struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
								addr = &(ipv4->sin_addr);
								ipver = "IPv4";

								// convert the IP to a string
								inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr)); //ipstr現在是個char陣列的頭指標，提供給inet_ntop函式去幫我查詢之後，把查到的IP位置儲存起來
								//printf(" %s: %s\n", ipver, ipstr);
								message_to_reply = "address get from domain name : " + std::string(ipstr) + "\n\n"; //因為ipstr是char陣列，所以需要用string的Constructor來轉換成string

								break; //可能會有多個IPv4 Addr（也有可能有多個IPv6），我就用第一個就好
							}
							// 我只想提供IPv4就好
							// else { // IPv6
							// 	struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
							// 	addr = &(ipv6->sin6_addr);
							// 	ipver = "IPv6";
							// }

							// // convert the IP to a string
							// inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));//ipstr現在是個char陣列的頭指標，提供給inet_ntop函式去幫我查詢之後，把查到的IP位置儲存起來
							// //printf(" %s: %s\n", ipver, ipstr);
							// message_to_reply = std::string(ipstr) + "\n\n";//因為ipstr是char陣列，所以需要用string的Constructor來轉換成string
						}

						freeaddrinfo(receivedDNSinfo); // 釋放這個鏈結串列

						// 以下兩行放到上面那個for迴圈的前面去，用黃稚存所教的「放前面等於default」的概念，這樣就不用if，也可能免去一些煩惱
						// if (message_to_reply.length() == 0)
						// 	message_to_reply = "Such DNS IP not found\n\n"; //助教可能很賤，故意給一個根本找不到IPAddr的webAddr，所以這裡補強一下。因為message_to_reply是每次while的iteration重新宣告，所以就用長度為零去判斷即可。
					}

					state = IDLE;

					std::cout << "ServerSelfStatus: Request DNS from user Complete." << std::endl;
				}
			}

			//現在需要把string轉換成char字串，才能傳給Client
			char *cstr = new char[message_to_reply.length() + 1];
			strcpy(cstr, message_to_reply.c_str());

			// std::cout << std::endl << std::endl << std::endl;
			// for(int i=0;i<message_to_reply.length(); i++) std::cout << cstr[i];
			// std::cout << std::endl << std::endl << std::endl;

			send(forClientSockfd, /*message_to_reply*/ cstr, /*sizeof(message_to_reply)*/ message_to_reply.length() + 1, 0);
			delete[] cstr;
			memset(inputBuffer, '\0', sizeof(inputBuffer));
			recv(forClientSockfd, inputBuffer, sizeof(inputBuffer), 0);
			//printf("Client says: %s\n", inputBuffer); //Client回傳： 有可能是各種各種上面那一段備註所列的其中一項，所以在迴圈裡面進行處理
		}

		std::cout << "\nServerSelfStatus: Connection ended.\nServerSelfStatus: Waiting for new connection...\n\n";
	}
	//現在這裡就是負責和Client互通有無。
	/*----------------------------------------------------------*/

	//Server端，不需要close。只有Client端才需要Close的函式。（因為Server端都是用Ctrl+C去中斷）

	return 0;
}