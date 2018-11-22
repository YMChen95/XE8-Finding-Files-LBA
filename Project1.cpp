//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include <time.h>
bool isLeapYear(int year)
{
	return ((year%4==0 && year%100!=0) || year%400==0);
}
// 以公元 1 年 1 月 1 日为基准，计算经过的日期
int getDays(int year, int month, int day)
{
	int m[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
	if(isLeapYear(year))
		m[2]++;
	int result = 0;
	for(int i = 1;i < year;i++)
	{
		result += 365;
		if(isLeapYear(i))
			result ++;
	}
	for(int i = 1;i < month;i++)
	{
		result += m[i];
	}
	result += day;

	return result;
}
int dayDis (int year1, int month1, int day1,
			int year2, int month2, int day2)
{
	return abs(getDays(year2, month2, day2) - getDays(year1, month1, day1));
}
//---------------------------------------------------------------------------
USEFORM("Unit1.cpp", Form1);
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	try
	{
		Application->Initialize();
		time_t t = time(0);
		char year[5];
		char month[3];
		char day[3];
		strftime( year, sizeof(year), "%Y",localtime(&t) );
		strftime( month, sizeof(month), "%m",localtime(&t) );
		strftime( day, sizeof(day), "%d",localtime(&t) );
		int daysDiff =dayDis(2018,10,22,StrToInt(year),StrToInt(month),StrToInt(day));
		if (daysDiff<=30) {
			Application->MainFormOnTaskBar = true;
			Application->CreateForm(__classid(TForm1), &Form1);
			Application->Run();
		}
		else{
			 //MessageBox("这是一个确定 取消的消息框！","标题", MB_OKCANCEL );
		}

	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
