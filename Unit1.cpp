//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
unsigned char* buffer_temp;

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DriveComboBox1Change(TObject *Sender)
{
	this->DirectoryListBox1->Drive=this->DriveComboBox1->Drive;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DirectoryListBox1Change(TObject *Sender)
{
	this->FileListBox1->Directory=this->DirectoryListBox1->Directory;

}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject *Sender)
{
	Edit1->Clear();
	Edit2->Clear();
	Edit3->Clear();
	//TODO: 计算\的个数(完成),分割文件名（完成）
	int numOfSlash =0;
	TCHAR StrDrive=this->DriveComboBox1->Drive;
	AnsiString StrDirectory = this->FileListBox1->FileName;
	char ListDirectory[64];
	char fileName[64][64]={""}; //指定分隔后子字符串存储的位置，这里定义二维字符串数组

	if (StrDirectory == "") {
		StrDirectory = this->FileListBox1->Directory;
	}
	strcpy(ListDirectory, StrDirectory.c_str());
	numOfSlash = calc_slash(ListDirectory,'\\');

	const char * split = "\\";
	char * p;
	int file_pos=0;
	p = strtok (ListDirectory,split);
	while(p!=NULL) {
		strcpy(fileName[file_pos], p);
		p = strtok(NULL,split);
		file_pos+=1;
	}
	Edit1->Text = StrDirectory;

	/*
	1.跳转$boot区，Function:判断MBR,GPT(Done) ->提取指定位置 & HextoInt（Done）
	2.跳转至MFT$0->MFT$5号(Done）
	3.提取指定位置信息（A0属性,runlist）(Done)
	TODO:4.loop
	5.外部索引项(INDX) 	5.1 Loop提取文件名(Done)
						5.2 索引项文件名与真实文件名对比确定位置 (Done)
						5.3  跳转至索引项所对应文件MFT号（Done）
	TODO:6. 判断80,90w/oA0, 90w/A0, 相对应
	Runlist->result, file name->mft#, RunList->Indx*/

	int offset_temp = 0;
	int phy_to_logi=0; //physical, logical sector 偏移

	fileHandle=OpenDeviceHANDLE(StrDrive);
	buffer_temp = new char [4096];
	Rdsec_SPTI(fileHandle,offset_temp,buffer_temp,1);
	//$boot区
	unsigned char* jumpI_range = GetRange(buffer_temp,454,2);
	int possible_offset = HextoDec(jumpI_range,2);

	memset(buffer_temp,0,sizeof(buffer_temp));
	memset(jumpI_range,0,sizeof(jumpI_range));
	if (possible_offset == 1) {//GPT
		int count=0;
		while(1){
			Rdsec_SPTI(fileHandle,2,buffer_temp,2);
			jumpI_range = GetRange(buffer_temp,32+count*128,8);
			offset_temp = HextoDec(jumpI_range,8);
			memset(jumpI_range,0,sizeof(jumpI_range));
			Rdsec_SPTI(fileHandle,offset_temp,buffer_temp,2);
			jumpI_range = GetRangeForName(buffer_temp,0,3);
			if(HextoDec(jumpI_range,3)==15422096){
				phy_to_logi= offset_temp;
				break;
			}
			else{
				memset(jumpI_range,0,sizeof(jumpI_range));
                memset(buffer_temp,0,sizeof(buffer_temp));
				count++;
			}
		}
	}
	else{//MBR
		phy_to_logi =  possible_offset;
		offset_temp =  phy_to_logi；
	}
	if(file_pos==1){
			Edit2->Text = phy_to_logi;
			Edit3->Text = 0;
	}
	else{
		//跳转$MFT 0号
		Rdsec_SPTI(fileHandle,offset_temp,buffer_temp,1);
		jumpI_range = GetRange(buffer_temp,48,8);//取MFT$0起始簇号 ->读取
		int MFTZero_Lba=8*HextoDec(jumpI_range,8)+phy_to_logi;
		offset_temp = MFTZero_Lba+5*2; //直接加5个LBA到5号
		memset(buffer_temp,0,sizeof(buffer_temp));
		memset(jumpI_range,0,sizeof(jumpI_range));
		Rdsec_SPTI(fileHandle,offset_temp,buffer_temp,4);//跳转到$MFT5号 ->读取

		//找A0 runlist属性
		int RunListHead_offset = 0;
		for (int i = 55; i < 256; i++) {
			 jumpI_range = GetRange(buffer_temp,i*4,4);
			 if(HextoDec(jumpI_range,4)==0){
				memset(jumpI_range,0,sizeof(jumpI_range));
				jumpI_range = GetRange(buffer_temp,(i+1)*4,4);
				if(HextoDec(jumpI_range,4)==160){
					memset(jumpI_range,0,sizeof(jumpI_range));
					jumpI_range = GetRange(buffer_temp,(i+1)*4+32,1);
					int RunList_offset = HextoDec(jumpI_range,1);
					memset(jumpI_range,0,sizeof(jumpI_range));
					RunListHead_offset =  (i+1)*4+RunList_offset;
					jumpI_range = GetRange(buffer_temp,RunListHead_offset,1);
					break;
				}
				else{
					memset(jumpI_range,0,sizeof(jumpI_range));
					continue;
				}
			 }
			 else{
				continue;
			 }
		}

		int firstpart = jumpI_range[0] >> 4;
		int secondpart = jumpI_range[0]-firstpart*16;
		memset(jumpI_range,0,sizeof(jumpI_range));
		jumpI_range = GetRange(buffer_temp,RunListHead_offset+firstpart+1,secondpart);//从runlist得到外部索引起始簇号

		offset_temp = HextoDec(jumpI_range,secondpart)*8+phy_to_logi;//索引项的cluster #(physical)
		Rdsec_SPTI(fileHandle,offset_temp,buffer_temp,4);//跳转到外部索引 ->读取
		memset(jumpI_range,0,sizeof(jumpI_range));

		jumpI_range=GetRange(buffer_temp,24,4);

		bool find_file_flag = false;
		int indx_temp_offset = HextoDec(jumpI_range,4)+24;//第一个索引项的偏移0x18
		memset(jumpI_range,0,sizeof(jumpI_range));

		while (!find_file_flag) {
			int indxLength =0, fileNameLength=0;
			jumpI_range=GetRange(buffer_temp,indx_temp_offset+8,2);// 索引项长度的偏移0x08
			indxLength = HextoDec(jumpI_range,2);
			memset(jumpI_range,0,sizeof(jumpI_range));

			int temp =indx_temp_offset + 82;
			jumpI_range=GetRange(buffer_temp,temp-2,1);
			fileNameLength = HextoDec(jumpI_range,1);
			memset(jumpI_range,0,sizeof(jumpI_range));

			char *fileNameIndx = new char[fileNameLength];
			jumpI_range=GetRangeForName(buffer_temp,temp,2*fileNameLength);
			int i = 0;
			int j =0;
			for (i = 0; i < 2*fileNameLength; i++) {
				if(jumpI_range[i]==0){
					continue;
				}
				else{
				   fileNameIndx[j]=  jumpI_range[i];
				   j++;
				}
			}
            //找到索引项对应
			if (strcmp(fileNameIndx,fileName[1])==0) {
				memset(jumpI_range,0,sizeof(jumpI_range));
				jumpI_range=GetRange(buffer_temp,indx_temp_offset,4);
				find_file_flag=true;
				offset_temp = MFTZero_Lba + HextoDec(jumpI_range,4)*2;
				if(file_pos==2){
					Edit2->Text =  offset_temp;
					Edit3->Text =  offset_temp-phy_to_logi;
				}
				else{
					int Physical_Lba_offset = GetPhysicalLBA(file_pos, offset_temp,MFTZero_Lba,fileName);
				}
			}
			else{
			   indx_temp_offset+= indxLength;
			   memset(jumpI_range,0,sizeof(jumpI_range));
			}

		}
	}
	//Edit3->Text = secondpart;
	//Edit3->Text =;

	/*
	Application->NormalizeTopMosts();
	#ifdef _DELPHI_STRING_UNICODE
	Application->MessageBox(L"This should be on top.", L"Look", MB_OKCANCEL);
	#else
	Application->MessageBox("This should be on top.", "Look", MB_OKCANCEL);
	#endif
	Application->RestoreTopMosts();
	*/

}
//---------------------------------------------------------------------------
HANDLE TForm1::OpenDeviceHANDLE(TCHAR cDriveLetter){

	LPTSTR szVolumeFormat = TEXT("\\\\.\\%c:");
	LPTSTR szRootFormat = TEXT("%c:\\");
	LPTSTR szErrorFormat = TEXT("Error %d: %s\n");
    DWORD accessMode = 0, shareMode = 0;
	UCHAR string[25];
	CHAR printftemp[100];
	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;  // default
    accessMode = GENERIC_WRITE | GENERIC_READ;       // default
	HANDLE hVolume;
    UINT uDriveType;
    TCHAR szVolumeName[8];
    TCHAR szRootName[5];
    wsprintf(szRootName,szRootFormat,cDriveLetter);

	   wsprintf(szVolumeName,szVolumeFormat,cDriveLetter);
 //   strcpy(string,"\\\\.\\I:");
    hVolume = CreateFile(szVolumeName,
                           accessMode,
                            shareMode,
                                 NULL,
						OPEN_EXISTING,
                                    0,
                                NULL);
         return hVolume;
}

/******************************************************************
		Fucntion Name: Rdsec()
		Command Code:
		Description:
******************************************************************/
BOOL TForm1::Rdsec_SPTI(HANDLE DeviceHandle,int LBA,unsigned char* wbuf,int Sec_Count){
	  BOOL status = 0;
	  SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
	  SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
      UCHAR string[25];
      ULONG length = 0,
      returned = 0;
 ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    sptdwb.sptd.SenseInfoLength = 24;
	sptdwb.sptd.DataTransferLength = (Sec_Count*512);//sectorSize;
	sptdwb.sptd.TimeOutValue = 2;
	sptdwb.sptd.DataBuffer = wbuf;
    sptdwb.sptd.SenseInfoOffset =
       offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    sptdwb.sptd.Cdb[0] =0x28;//operation Code
    sptdwb.sptd.Cdb[1] =0x00;
    sptdwb.sptd.Cdb[2]=(UCHAR)(LBA >> 24);   // Logical Block Address [MSB->LSB]
    sptdwb.sptd.Cdb[3]=(UCHAR)(LBA >> 16);
    sptdwb.sptd.Cdb[4]=(UCHAR)(LBA >> 8); //
    sptdwb.sptd.Cdb[5]=(UCHAR)(LBA);   //
    sptdwb.sptd.Cdb[6]=00;     //Reserved
    sptdwb.sptd.Cdb[7]=0x00;    // Sector Count
    sptdwb.sptd.Cdb[8]=Sec_Count;    // Sector Count
    length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	status = DeviceIoControl(DeviceHandle,
							 IOCTL_SCSI_PASS_THROUGH_DIRECT,
                             &sptdwb,
							 length,
							 &sptdwb,
                             length,
                             &returned,
                             FALSE);
        return TRUE;
}

int TForm1::calc_slash(const char *p, const char chr)
{
	int count = 0;
	while(*p)
	{
		if(*p == chr)//解引用取出字符与chr比较，指针本身改变
			++count;
		++p;//使用指针指向每个字符所在的内存位地址
	}
	return count;
}
/*
int TForm1::RunListStartLba(char runlistHead)
{
	int StartLba = 0;

	return StartLba;
}
  */
unsigned char* TForm1::GetRange(unsigned char* p, int a, int b)
{
	unsigned char*temp_p = new unsigned char[b+1];

	for (int i = 0; i < b; i++) {
		temp_p[i]=p[a+b-1-i];
	}
	return temp_p;
}

unsigned char* TForm1::GetRangeForName(unsigned char* p, int a, int b)
{
	unsigned char*temp_p = new unsigned char[b+1];

	for (int i = 0; i < b; i++) {
		temp_p[i]=p[a+i];
	}
	return temp_p;
}

unsigned long TForm1::HextoDec(const unsigned char *hex, int length)
{
	int i;
	unsigned long rslt = 0;
	for (i = 0; i < length; i++)
	{
		rslt += (unsigned long)(hex[i]) << (8 * (length - 1 - i));
	}
	return rslt;
}

int TForm1::GetPhysicalLBA(int filepos, int offset_temp, int MFTZero_Lba, char fileName[64][64])
{     /*TODO 	1.在文件夹中先查找0x80和0x90属性头，0x04偏移得到属性长度
				2.查找0x90属性结束位置有没有0xA0属性

		*/
	int result =0;
	buffer_temp = new char [4096];
	Rdsec_SPTI(fileHandle,offset_temp,buffer_temp,2);

	bool Azero_exsit = false;
	for (int i = 20; i < 256; i++) {
		unsigned char* jumpI_range = GetRange(buffer_temp,i*4,4);
		if(HextoDec(jumpI_range,4)==144){//90属性
			memset(jumpI_range,0,sizeof(jumpI_range));
			jumpI_range = GetRange(buffer_temp,(i+1)*4,4);
			int Azero_offset =  i*4+HextoDec(jumpI_range,4);
			memset(jumpI_range,0,sizeof(jumpI_range));
			jumpI_range = GetRange(buffer_temp,Azero_offset,4);
			if(HextoDec(jumpI_range,4)==160){//A0属性
			   Azero_exsit = true;
			}
			//Edit2->Text = jumpI_range[3];
			break;
		}
		else if(HextoDec(jumpI_range,4)==128){//80属性
			memset(jumpI_range,0,sizeof(jumpI_range));
			jumpI_range = GetRange(buffer_temp,40,1);
			int firstpart = jumpI_range[0] >> 4;
			int secondpart = jumpI_range[0]-firstpart*16;
			memset(jumpI_range,0,sizeof(jumpI_range));
			jumpI_range = GetRange(buffer_temp,i+40+firstpart+1,secondpart);

			break;
		}
		else{
			memset(jumpI_range,0,sizeof(jumpI_range));
			continue;
		}

	}
	Edit3->Text = fileName[filepos-1];

	return result;
}




void __fastcall TForm1::FileListBox1Change(TObject *Sender)
{
	this->FileListBox1 ->Directory=this->FileListBox1 ->Directory;
}
//---------------------------------------------------------------------------


