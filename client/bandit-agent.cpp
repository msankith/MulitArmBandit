#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "betaDistributionLib.hpp"
#define MAXHOSTNAME 256
#define lin long long int
using namespace std;

void options(){

  cout << "Usage:\n";
  cout << "bandit-agent\n"; 
  cout << "\t[--numArms numArms]\n";
  cout << "\t[--randomSeed randomSeed]\n";
  cout << "\t[--horizon horizon]\n";
  cout << "\t[--hostname hostname]\n";
  cout << "\t[--port port]\n";
}


/*
  Read command line arguments, and set the ones that are passed (the others remain default.)
*/
bool setRunParameters(int argc, char *argv[], int &numArms, int &randomSeed, unsigned long int &horizon, string &hostname, int &port){

  int ctr = 1;
  while(ctr < argc){

    //cout << string(argv[ctr]) << "\n";

    if(string(argv[ctr]) == "--help"){
      return false;//This should print options and exit.
    }
    else if(string(argv[ctr]) == "--numArms"){
      if(ctr == (argc - 1)){
	return false;
      }
      numArms = atoi(string(argv[ctr + 1]).c_str());
      ctr++;
    }
    else if(string(argv[ctr]) == "--randomSeed"){
      if(ctr == (argc - 1)){
	return false;
      }
      randomSeed = atoi(string(argv[ctr + 1]).c_str());
      ctr++;
    }
    else if(string(argv[ctr]) == "--horizon"){
      if(ctr == (argc - 1)){
	return false;
      }
      horizon = atoi(string(argv[ctr + 1]).c_str());
      ctr++;
    }
    else if(string(argv[ctr]) == "--hostname"){
      if(ctr == (argc - 1)){
	return false;
      }
      hostname = string(argv[ctr + 1]);
      ctr++;
    }
    else if(string(argv[ctr]) == "--port"){
      if(ctr == (argc - 1)){
	return false;
      }
      port = atoi(string(argv[ctr + 1]).c_str());
      ctr++;
    }
    else{
      return false;
    }

    ctr++;
  }

  return true;
}



int thompsonSampling_getArm(int numArms,lin alpha[],lin beta[])
{ 
  int bestArm=0;
  double bestMean=0;
  for(int i=0;i<numArms;i++)
  {
    std::random_device rd;
    std::mt19937 gen(rd());
    sftrabbit::beta_distribution<> betaDist(alpha[i]+1, beta[i]+1);
    double temp=betaDist(gen);
    if(temp>bestMean)
    {
      bestMean=temp;
      bestArm=i;
    }
  }
  return bestArm;
}


int tau(float alpha,int r)
{
return (int)ceil(pow(1+alpha,r));
}

int UCB2_getArm(int rounds,int numArms,float armMean[],int armPlayed[],float alpha)
{
  int bestArm=0;
  double bestMean=0;
  for(int i=0;i<numArms;i++)
  {
    float temp=armMean[i];
    temp+=sqrt(((1+alpha)*log((2.718*rounds)/tau(alpha,armPlayed[i])))/(2*tau(alpha,armPlayed[i])));
    cout<<"Arm "<<i<<"  "<<temp<<"  tau "<<tau(alpha,armPlayed[i])<<endl;
    //temp+=sqrt(((1+alpha)*log(exp(rounds)/tau(alpha,armPlayed[i])))/(2*tau(alpha,armPlayed[i])));
    if (temp>bestMean)
    {
      bestArm=i;
      bestMean=temp;
    }
  }
 return bestArm;
}

int main(int argc, char *argv[]){
  
  // Run Parameter defaults.
  int numArms = 5;
  int randomSeed = time(0);
  unsigned long int horizon = 1000;
  string hostname = "localhost";

  int port = 5000;

  //Set from command line, if any.
  if(!(setRunParameters(argc, argv, numArms, randomSeed, horizon, hostname, port))){
    //Error parsing command line.
    options();
    return 1;
  }

  //Thompson distribution intialization 
  lin *alphaValues,*betaValues;
  alphaValues=(lin *)malloc(sizeof(lin)*numArms);
  betaValues=(lin *)malloc(sizeof(lin)*numArms);
  for(int i=0;i<numArms;i++)
  {
    alphaValues[i]=2;
    betaValues[i]=2;
  }

  //UCB Algorithm initialization
  float *armMean;
  float UCBalpha=0.002;
  int *armPlayed;
  armMean=(float *)malloc(sizeof(float)*numArms);
  armPlayed=(int *)malloc(sizeof(float)*numArms);

  struct sockaddr_in remoteSocketInfo;
  struct hostent *hPtr;
  int socketHandle;

  bzero(&remoteSocketInfo, sizeof(sockaddr_in));
  
  if((hPtr = gethostbyname((char*)(hostname.c_str()))) == NULL){
    cerr << "System DNS name resolution not configured properly." << "\n";
    cerr << "Error number: " << ECONNREFUSED << "\n";
    exit(EXIT_FAILURE);
  }

  if((socketHandle = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    close(socketHandle);
    exit(EXIT_FAILURE);
  }

  memcpy((char *)&remoteSocketInfo.sin_addr, hPtr->h_addr, hPtr->h_length);
  remoteSocketInfo.sin_family = AF_INET;
  remoteSocketInfo.sin_port = htons((u_short)port);

  if(connect(socketHandle, (struct sockaddr *)&remoteSocketInfo, sizeof(sockaddr_in)) < 0){
    close(socketHandle);
    exit(EXIT_FAILURE);
  }


  char sendBuf[256];
  char recvBuf[256];
  int thompsonSampling=1;
  int UCBSample=0;
  int UCBThompsonSample=1;
  //if(horizon>5000)
   // thompsonSampling=0;
cout<<"------------------------------"<<endl;
if(thompsonSampling){ 
  int rounds=0;
  while(rounds<horizon){
    int armToPull=0;
    float reward;
    if(rounds<numArms)
      armToPull=rounds;
    else
      armToPull=thompsonSampling_getArm(numArms,alphaValues,betaValues);
    sprintf(sendBuf, "%d", armToPull);
    cout << "Sending action " << armToPull << ".\n";
    if(send(socketHandle, sendBuf, strlen(sendBuf)+1, MSG_NOSIGNAL) <0)
      break;

    recv(socketHandle, recvBuf, 256, 0);
    sscanf(recvBuf, "%f", &reward);
    cout << "Received reward " << reward << ".\n";
    if(rounds<numArms)
    {
      armMean[rounds]=reward/1;
      armPlayed[rounds]=1;
    }

    if(reward==1)
      alphaValues[armToPull]+=1;
    else
      betaValues[armToPull]+=1;
    rounds++;
 } 
}
else if(UCBSample){
cout<<"UCB SAMPLE"<<endl;
  int rounds=0;
  int *UCBArmPlayed=(int *)malloc(sizeof(int)*numArms);
  while(rounds<horizon){
    int armToPull=0;
    float reward;
    if(rounds<numArms)
      armToPull=rounds;
    else
      armToPull=UCB2_getArm(rounds,numArms,armMean,armPlayed,UCBalpha);
      //armToPull=thompsonSampling_getArm(numArms,alphaValues,betaValues);
    if(rounds<numArms)
    {
      sprintf(sendBuf, "%d", armToPull);
      cout << "Sending action " << armToPull << ".\n";
      if(send(socketHandle, sendBuf, strlen(sendBuf)+1, MSG_NOSIGNAL) <0)
        break;

      recv(socketHandle, recvBuf, 256, 0);
      sscanf(recvBuf, "%f", &reward);
      cout << "Received reward " << reward << ".\n";
      UCBArmPlayed[rounds]=1;
      armMean[rounds]=reward/1;
      armPlayed[rounds]=1;
      rounds++;
    }
    else
    {
	int itr=tau(UCBalpha,armPlayed[armToPull]+1)-tau(UCBalpha,armPlayed[armToPull]);
	int temp=itr;
        int cumulativeReward=0;
	cout<<"Round ===== "<< itr <<endl;
	while(temp--)
        {
		float reward;
                 sprintf(sendBuf, "%d", armToPull);
     		 cout << "Sending action " << armToPull << ".\n";
     		 if(send(socketHandle, sendBuf, strlen(sendBuf)+1, MSG_NOSIGNAL) <0)
        		break;

      		recv(socketHandle, recvBuf, 256, 0);
      		sscanf(recvBuf, "%f", &reward);
      		cout << "Received reward " << reward << ".\n";
		cumulativeReward+=reward;
		
	}
        armMean[armToPull]=(armMean[armToPull]*UCBArmPlayed[armToPull]+cumulativeReward)/(UCBArmPlayed[armToPull]+itr);
	//armMean[armToPull]=(armMean[armToPull]*armPlayed[armToPull]+cumulativeReward)/(armPlayed[armToPull]+1);
	armPlayed[armToPull]+=1;
	rounds+=itr;
    }
    /*
    if(reward==1)
      alphaValues[armToPull]+=1;
    else
      betaValues[armToPull]+=1;
    rounds++;
     */
 }
}
else  if(UCBThompsonSample){
cout<<"UCB SAMPLE"<<endl;
  int rounds=0;
  while(rounds<horizon){
    int armToPull=0;
    float reward;
    if(rounds<numArms)
      armToPull=rounds;
    else
      {//armToPull=UCB2_getArm(rounds,numArms,armMean,armPlayed,UCBalpha);
      armToPull=thompsonSampling_getArm(numArms,alphaValues,betaValues);
      }
     if(rounds<numArms)
    {
      sprintf(sendBuf, "%d", armToPull);
      cout << "Sending action " << armToPull << ".\n";
      if(send(socketHandle, sendBuf, strlen(sendBuf)+1, MSG_NOSIGNAL) <0)
        break;

      recv(socketHandle, recvBuf, 256, 0);
      sscanf(recvBuf, "%f", &reward);
      cout << "Received reward " << reward << ".\n";
      armMean[rounds]=reward/1;
      armPlayed[rounds]=1;
      if(reward==1)
         alphaValues[armToPull]+=1;
      else
         betaValues[armToPull]+=1;
       rounds++;
      
    }
    else
    {
        int itr=tau(UCBalpha,armPlayed[armToPull]+1)-tau(UCBalpha,armPlayed[armToPull]);
        int temp=itr;
        int cumulativeReward=0;
        cout<<"Round ===== "<< itr <<endl;
        while(temp&&rounds+temp--<horizon)
        {
                float reward;
                 sprintf(sendBuf, "%d", armToPull);
                 cout << "Sending action " << armToPull << ".\n";
                 if(send(socketHandle, sendBuf, strlen(sendBuf)+1, MSG_NOSIGNAL) <0)
                        break;

                recv(socketHandle, recvBuf, 256, 0);
                sscanf(recvBuf, "%f", &reward);
                cout << "Received reward " << reward << ".\n";
                cumulativeReward+=reward;
		if(reward==1)
         	  alphaValues[armToPull]+=1;
      		else
         	  betaValues[armToPull]+=1;
      
	
        }
        armMean[armToPull]=(armMean[armToPull]*armPlayed[armToPull]+cumulativeReward)/(armPlayed[armToPull]+1);
        armPlayed[armToPull]+=1;
        rounds+=itr;
    }
    /*
    if(reward==1)
      alphaValues[armToPull]+=1;
    else
      betaValues[armToPull]+=1;
    rounds++;
     */
 }
}

else
{
  int armToPull = 0;
  sprintf(sendBuf, "%d", armToPull);
  cout << "Sending action " << armToPull << ".\n";

  while(send(socketHandle, sendBuf, strlen(sendBuf)+1, MSG_NOSIGNAL) >= 0){

    float reward = 0;

    recv(socketHandle, recvBuf, 256, 0);
    sscanf(recvBuf, "%f", &reward);
    cout << "Received reward " << reward << ".\n";

    armToPull = (armToPull + 1) % numArms;
    sprintf(sendBuf, "%d", armToPull);
    cout << "Sending action " << armToPull << ".\n";
  }
  
}
  close(socketHandle);
  cout << "Terminating.\n";

}
          
