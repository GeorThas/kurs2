//---------------------------------------------------------------------------
#ifndef MainFormH
#define MainFormH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Jpeg.hpp>
#include "GifImg.hpp"
#include <VCLTee.Chart.hpp>
#include <VclTee.TeeGDIPlus.hpp>
#include <VCLTee.TeEngine.hpp>
#include <VCLTee.TeeProcs.hpp>
#include <Vcl.Dialogs.hpp>
#include <VCLTee.Series.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.ExtDlgs.hpp>
#include <System.ImageList.hpp>
#include <Vcl.ImgList.hpp>
//---------------------------------------------------------------------------

class TMForm : public TForm
{
__published:	// IDE-managed Components
	TStringGrid *Table;
	TMemo *MemoForDataOutput;
	TButton *bDataProcBtn;
	TLabel *Label;
	TChart *ChartForDist;
	TOpenDialog *OpenDialog1;
	TButton *bAddCol;
	TStatusBar *StatusBar;
	TSaveDialog *SaveDialog1;
	TMainMenu *MainMenu1;
	TMenuItem *mSave;
	TMenuItem *mSettings;
	TMenuItem *mFile;
	TMenuItem *mFileOpen;
	TButton *bDelDiapCols;
	TButton *bInsCol;
	TCheckBox *cbSelDiap;
	TMenuItem *mCellAutoWidth;
	TBarSeries *Series1;
	TMenuItem *mFontSettings;
	TFontDialog *FontDialog1;
	TMenuItem *mCreate;
	TMenuItem *mSaveAs;
	TMenuItem *mExit;
	TMenuItem *mResults;
	TMenuItem *N2;
	TMenuItem *mResSaveAs;
	TBarSeries *Series2;
	TImageList *IconsList;
	TMenuItem *mSaveImgAs;
	void __fastcall mFileOpenClick(TObject *Sender);
	void __fastcall bCalcDrawClick(TObject *Sender);
	void __fastcall bAddColClick(TObject *Sender);
	void __fastcall FResize(TObject *Sender);
	void __fastcall TableKeyPress(TObject *Sender, System::WideChar &Key);
	void __fastcall TableSetEditText(TObject *Sender, int ACol, int ARow, const UnicodeString Value);
	void __fastcall mSaveClick(TObject *Sender);
	void __fastcall cbSelDiapClick(TObject *Sender);
	void __fastcall bDelDiapColsClick(TObject *Sender);
	void __fastcall bInsColClick(TObject *Sender);
	void __fastcall TableSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect);
	void __fastcall mCellAutoWidthClick(TObject *Sender);
	void __fastcall TableGetEditText(TObject *Sender, int ACol, int ARow, UnicodeString &Value);
	void __fastcall TableDblClick(TObject *Sender);
	void __fastcall TableDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect,
		  TGridDrawState State);
	void __fastcall CloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall mFontSettingsClick(TObject *Sender);
	void __fastcall mSaveAsClick(TObject *Sender);
	void __fastcall mCreateClick(TObject *Sender);
	void __fastcall FontDialog1Apply(TObject *Sender, HWND Wnd);
	void __fastcall mExitClick(TObject *Sender);
	void __fastcall mResSaveAsClick(TObject *Sender);
	void __fastcall BMPClick(TObject *Sender);
	void __fastcall JPEGClick(TObject *Sender);
	void __fastcall GIFClick(TObject *Sender);
	void __fastcall OpenDialog1Show(TObject *Sender);
	void __fastcall mSaveImgAsClick(TObject *Sender);
	void __fastcall SaveDialog1CanClose(TObject *Sender, bool &CanClose);
	void __fastcall SaveDialog1Show(TObject *Sender);


private:	// User declarations

	UnicodeString FCurCellText;
	int FSortedColNbr;
	bool __fastcall CheckGridCell(int col, int row);
	bool __fastcall CheckChangesAndSave();
	bool __fastcall SaveData(bool showDialog);
    void __fastcall sgFontChanged(System::TObject* Sender);
public:		// User declarations
	__fastcall TMForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TMForm *MForm;
//---------------------------------------------------------------------------
#endif
