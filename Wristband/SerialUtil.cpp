//SerialUtil Library Created by SHUBHAM VERMA
#include "SerialUtil.h"	// Library header to be included
Serial* SerialUtil::SP=NULL;//Pointer to SerialClass
SerialUtil::SerialUtil()
{
	cout<<":::::::::::SERIAL PORT COMMUNICATION TUTORIAL:::::::::::\n\n";

	incomingData[0]='\0';
	incomingDataLength = 256;
	readResult = 0;
	receive_command="";

	strcpy_s(outgoingData,"");
	outgoingDataLength = 256;

	clearedRX=false;

	strcpy_s(port,"\\\\.\\COM4");		//default port is COM4
	SP = new Serial(this->port);    // adjust as needed

	if (SP->IsConnected())
		cout<<"..Connected to Serial Port COM4(default)\n";
	else 
		throw "Failed to open Serial Port!!";
}
SerialUtil::SerialUtil(string port)
{
	cout<<":::::::::::SERIAL PORT COMMUNICATION TUTORIAL:::::::::::\n\n";

	incomingData[0]='\0';
	incomingDataLength = 256;
	readResult = 0;
	receive_command="";

	strcpy_s(outgoingData,"");
	outgoingDataLength = 256;

	clearedRX=false;

	strcpy_s(this->port,port.c_str());
	SP = new Serial(this->port);    // adjust as needed

	if (SP->IsConnected())
		cout<<"..Connected to Serial Port "<<port<<"\n";
	else 
		throw "Failed to open Serial Port!!";
}
string SerialUtil::read()
{
	string read="";

	readResult = SP->ReadData(incomingData,incomingDataLength);
	receive_command=(incomingData);
	strcpy_s(incomingData,"");//clear buffer
	clearedRX=PurgeComm(  SP->getHandle(), PURGE_RXCLEAR);//clear RX
	Sleep(10);

	read=string(receive_command);
	return read;
}
void SerialUtil::write(char* str)
{
	if(SP->IsConnected())
	{
		strcpy_s(outgoingData, 256 ,str);
		clearedRX=PurgeComm(  SP->getHandle(), PURGE_RXCLEAR);//clear RX
		Sleep(10);
		SP->WriteData(outgoingData,outgoingDataLength);
		cout<<"outgoingData:"<<str<<"\n";
		if(SP->IsConnected())
		{
			/*
			//check until the received string is eaual to the sent string OR it is equalto BAD_COMMAND ("-" sent by Arduino) OR whether it is an empty string("").
			while((receive_command.compare(str)!=0&&receive_command.compare("-")!=0) || receive_command.compare("")==0)
			{
				readResult = SP->ReadData(incomingData,incomingDataLength);
				receive_command=(incomingData);
				strcpy_s(incomingData,"");//clear buffer
				clearedRX=PurgeComm(  SP->getHandle(), PURGE_RXCLEAR);//clear RX
				Sleep(10);			
			}*/
			//print suitable statements respectively
			if(receive_command.compare(str)==0)
				cout<<"incomingData:"<<str<<"\n";
			if(receive_command.compare("-")==0)
				cout<<"incomingData:"<<"BAD_COMMAND"<<"\n";
			if(receive_command.compare("")==0)
				cout<<"incomingData:NULL returned"<<receive_command<<"\n";
		}
		receive_command="";
		strcpy_s(incomingData,"\0");
		Sleep(10);
	}
}