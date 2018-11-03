//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.FileCtrl.hpp>

#include <winioctl.h>
#include "ntddscsi.h"
#include "spti.h"

//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TDirectoryListBox *DirectoryListBox1;
	TDriveComboBox *DriveComboBox1;
	TFileListBox *FileListBox1;
	TButton *Button1;
	TEdit *Edit1;
	TEdit *Edit2;
	TEdit *Edit3;
	TLabel *Label1;
	TLabel *Label2;
	void __fastcall DriveComboBox1Change(TObject *Sender);
	void __fastcall DirectoryListBox1Change(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
	HANDLE fileHandle;
	HANDLE OpenDeviceHANDLE(TCHAR cDriveLetter);
	BOOL Rdsec_SPTI(HANDLE DeviceHandle,int LBA,unsigned char* wbuf,int Sec_Count);
	int calc_slash(const char *p, const char chr);
	unsigned char* GetRange(unsigned char* p, int a, int b);
	unsigned long HextoDec(const unsigned char *hex, int length);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
