/* 參考資料網站： http://zake7749.github.io/2015/03/17/SocketProgramming/ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

int main(int argc, char *argv[])
{
	/*----------------------------------------------------------*/
	//本地端（a.k.a. Server端）的socket的建立
	int sockfd = 0;																																/*註冊Client本地的Socket之後，會收到的的描述符（描述符是啥，請見本code上方的參考資料網站，然後去查詢維基百科。其實就是類似open()所回傳的檔案標頭）*/
	sockfd = socket(AF_INET /*NET指涉是使用IPv4，而不是IPv6，也不是單純只能LocalHost執行，而是可以真正www傳遞*/, SOCK_STREAM /*指定用TCP*/, 0); //基本上這是固定用法，沒什麼好改動的
	//本地端（a.k.a. Server端）的Socket創建如果成功，sockfd這個描述符就不會是-1
	if (sockfd == -1)
	{
		printf("Fail to create a socket.");
		return 0;
	}
	else
		std::cout << "Socket Created!\n";
	std::cout << "SockFD is: " << sockfd << std::endl
			  << std::endl;
	//本地端（a.k.a. Client端）的socket的建立
	//！！目前只是指定了socket是TCP格式！！目前我們還沒有對所願意目前這個Client端說想要溝通的伺服器是哪個，進行指定！
	/*----------------------------------------------------------*/

	/*----------------------------------------------------------*/
	//現在要針對這個Client端，進行「想要連接的server是誰」的設定
	struct sockaddr_in info;
	bzero(&info, sizeof(info));
	info.sin_family = PF_INET;
	info.sin_addr.s_addr = inet_addr("127.0.0.1"); //助教指定要連接到localHost這個IPAddr
	info.sin_port = htons(1234);				   //助教指定要這個portNum
	//現在要針對這個Client端，進行「想要連接的server是誰」的設定
	/*----------------------------------------------------------*/

	/*----------------------------------------------------------*/
	//現在我這一端（a.k.a. Client端），要跟上面所設定好的伺服器，進行連線了！
	int err = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
	if (err == -1)
		printf("Connection error\n");
	else
		std::cout << "Connected to Server!\n";
	std::cout << "ErrCode is: " << err << std::endl
			  << std::endl;
	//現在我這一端（a.k.a. Client端），要跟上面所設定好的伺服器，進行連線了！
	/*----------------------------------------------------------*/

	/*--------------------*/
	bool isLeaving = false; // 只有在使用者輸入3， 並且當時Server的訊息是「What's your requirement? 1.DNS 2.QUERY 3.QUIT :」的時候，才能夠離開。這個flag主要是怕如果Client是在2的TABLE底下，如果輸入“3”，如果沒有這個flag就會離開程式。
	/*--------------------*/

	/*----------------------------------------------------------*/
	//現在這裡就是負責和Server互通有無、交換訊息。
	// char message[100]; message[0] = '.';//'.'代表Start
	std::string message;
	message = ".\0";
	char receiveMessage[100] = {};
	recv(sockfd, receiveMessage, sizeof(receiveMessage), 0);
	printf("%s", receiveMessage);	   //來自Server的：Hi! Ready for service!
	send(sockfd, "?", sizeof("?"), 0); //傳送"?"過去，Server就會知道我現在Client沒有要表示什麼，所以它就可以繼續講它想講的

	bool flag_lasttimeQuestioned = false; //這只是為了懶得抓code裡面的某個bug所用的簡單措施。
	while (!isLeaving)
	{
		message = ""; //把string型別的message清空，因為message是要從Client回傳回給Server的，所以我希望他每輪都可以從最乾淨的狀態開始

		//因為recv用receiveMessage是Buffer型態，所以比較舊的！！！！不會消失！！！！，只是會把它排到陣列的後方去而已。所以為了後面的這個Constructor（std::string str(receiveMessage);）不會吃到以前舊的我早就處理過的訊息，我就需要吧receiveMessage陣列給清空(正確來講，因為是char陣列，所以是清成'\0')
		memset(receiveMessage, '\0', sizeof(receiveMessage));
		recv(sockfd, receiveMessage, sizeof(receiveMessage), 0);
		if (receiveMessage[0] == 'W' || receiveMessage[0] == 'I')
			printf("%s", receiveMessage); //Server會傳過來的東西很多，可能是問What，也可能是問Input IP/Input URL。以上三種情況，是要直接print出來給使用者輸入。！！但，如果Server是回傳EMail或IPAddr，因為後面我才會進行處理和印出，所以這裡不能印！

		//因為Server傳過來的字串一定是W、I、a、E、「空行」開頭，所以這個來作為對Server所傳來的訊息的正確反應的判斷
		//只有W、I開頭的Server過來的訊息需要回覆，其他都不用使用者回覆（也就是我幫他們代回覆一個字元'?'即可！）！
		if (receiveMessage[0] == 'W' || receiveMessage[0] == 'I')
		{
			std::cin >> message;
			//如果使用者輸入網址，就不會出錯，所以下面的判斷式也不會對message字串造成任何影響。
			//但因為使用者可能輸入1～3的選項編號，也有可能輸入學號。所以我們需要針對這兩個，給出Server能夠判斷的標記
			//那因為1～3只有單個字元，所以如果使用者輸入1，我就把message字串變成"1/"，用"/"當作給Server的判斷符號。因為學號沒有單位數的。
			//順便偵錯，如果單位數，但不在1～3範圍內，我就給Server錯誤訊息"!"

			//??????????注意！scanf會因為讀取到LF(LineFeed)而切斷抓取，但是這個LF的char依然在系統buffer裡面！！！所以我需要把這個LF的char吃乾淨做收尾，否則每次使用者輸入一次，都會echo一次不正確的東西

			if (message.length() == 1)
			{
				//if(message != "1" && message != "2" && message != "3") message = "!";//雖然下面那段寫的洋洋灑灑，但如果使用者是要在Table階段查詢4的話，這樣寫會出錯，見else那邊的「！！！   ！！！」註解

				//經過上面的if之後，我們知道使用者目前輸入的是單位數的數字，而且不是1～3。照理來講，按照Server那邊的設計，那就應該後面各自加上'/'才能傳給Server。
				//但因為如果Client是輸入3，當時是在他剛剛伺服器問說「What's your requirement? 1.DNS 2.QUERY 3.QUIT :」那時候他如果回答2，而他現在伺服器進一步問說「Input student ID : 」他卻回答3。
				//如果我直接把3依照上上行所說的加上'/'變成“3/”傳給伺服器，伺服器會以為Client要斷線，但其實並不是！
				//所以要針對這個edge case做處理

				//如果是使用者是輸入1或2的話，就直接加上'/'
				/*else*/ if (message == "1" || message == "2")
					message += '/';
				else
				{ //現在是message == "3"的情況， ！！！或者是0，或4～9！！！
					if (message == "3")
					{
						if (receiveMessage[0] == 'W')
							message += '/';
						//因為現在是在if(receiveMessage[0] == 'W' || receiveMessage[0] == 'I') 的大框架底下，所以else的情形，就是指receiveMessage[0] == 'I'的情形
						else
							; //message不加上'/'，這樣Server那邊才會正確分流到查表那邊。
					}
					else
						; //0, 4~9，那就不動就好，就會自動分流到Server裡面Table那個分支
				}
			}

			if (message == "3/" && receiveMessage[0] == 'W')
				isLeaving = true;
		}
		//else message = "?";//‘？’代表Server不需要給出反應 //！！！！！！錯誤分流，因為除了W、I開頭以外，server還可能會回傳email地址或者IP位址！
		else
		{
			std::string str(receiveMessage); // 透過string的其中一種Constructor把收到的從Server那邊收到的東西轉換成string，這樣我比較好處理

			if (str.find("@") != std::string::npos)
			{ //如果Server回傳的是Email Addr
				std::cout << str;
			}
			else
			{ // 剩下兩種可能的情況，一種是Server告訴我一串IPAddr，那我就需要cout出來。另一種情況是Server回報了一個Client所造成的錯誤代碼（a.k.a. Server給出錯誤提示）。不論是哪個，我們把它cout出來即可。
				// //判斷是否是IPAddr
				// int num_of_dots = 0;
				// for (const auto &c: str) if(c == '.') num_of_dots++;
				// if (num_of_dots == 3) {
				// 	std::cout << str << std::endl;
				// }
				// else { // Server給出錯誤代碼(a.k.a. 錯誤提示)
				// 	std::cout << str << std::endl;
				// }
				std::cout << str;
			}
			message = "?";
		}

		//總共分成三種情況
		//1. 上次不是傳"?"，那就允許它傳
		//2. 上次是傳"?"，但這次它不是想要傳"?"，那就不用阻止它
		//3. 上次是傳"?"，這次又想要傳"?"，那就不准它傳
		if (!flag_lasttimeQuestioned)
		{ //情況1
			send(sockfd, /*message*/ message.c_str(), message.length() + 1 /* +1是因為考慮'\0'這個字元*/, 0);

			if (message == "?")
				flag_lasttimeQuestioned = true;
		}
		else
		{ //情況2、3，上次是傳"?"
			if (message != "?")
			{ //情況1
				send(sockfd, /*message*/ message.c_str(), message.length() + 1 /* +1是因為考慮'\0'這個字元*/, 0);

				flag_lasttimeQuestioned = false;
			}
			else
			{	//情況3
				//不允許傳
				//flag原本就是true了，現在要維持true
			}
		}
	}

	//現在這裡就是負責和Server互通有無、交換訊息。
	/*----------------------------------------------------------*/

	printf("Socket closed. The program is terminated.\n\n");
	close(sockfd);

	return 0;
}