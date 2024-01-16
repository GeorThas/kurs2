//---------------------------------------------------------------------------
#include <fstream>
#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMForm *MForm;
//---------------------------------------------------------------------------

const int MIN_COL_WIDTH = 12;
UnicodeString TITLE = "Расчет пробега";


class TMyStringGrid : TStringGrid {
	public:

	using TStringGrid::InvalidateCell;
};

int CalcGridColWidth(TStringGrid* grid, int colIdx){
	if (!grid || colIdx < 0 || colIdx >= grid->ColCount)
		return 0;
	grid->Canvas->Font->Assign(grid->Font);
	int maxWidth = 0;
	for (int i = 0; i < grid->RowCount; i++) {
		int txtWidth = grid->Canvas->TextWidth(grid->Cells[colIdx][i]);
		if (txtWidth > maxWidth)
			maxWidth = txtWidth;
	}
	return maxWidth + MIN_COL_WIDTH;
}

int AdjGridColWidths(TStringGrid *grid) {
	if(!grid)
		return 0;
	int Width = 0;
	for (int i = 0; i < grid->ColCount; i++) {
		int buf = CalcGridColWidth(grid, i);
		grid->ColWidths[i] = buf;
		Width += buf;
	}

	if (!(grid->Options * (TGridOptions() << goFixedHorzLine << goHorzLine)).Empty())
			Width += grid->ColCount * grid->GridLineWidth;
	return Width;
}
//--------------------------------------------------------------------------------------
typedef double TCmpFunc(const UnicodeString &s1, const UnicodeString &s2);

void SortGridByRow (TStringGrid *grid, int rowIdx, bool ascending, TCmpFunc cmpFunc){

    float d;
	for (int col = 1; col < grid->ColCount - 1; col++)
		for (int row = 1; row < grid->RowCount - 1; row++)
			if (!TryStrToFloat(grid->Cells[col][row], d) || d < 0)
				if (Application->MessageBox((UnicodeString(L"Таблица содержит недопустимые символы или пустые ячейки!\nСортировка невозможна!")).w_str(),
						Application->Title.w_str(), MB_OK|MB_ICONERROR))
					return;

	TStrings* row = grid->Rows[rowIdx];
	TStrings* tmpCol = new TStringList();

	try {
		for (int i = 1/*grid->FixedCols*/; i < grid->ColCount - 1; i++)
			for (int j = i+1; j < grid->ColCount; j++)
				if (cmpFunc(row->Strings[i], row->Strings[j]) > 0 && ascending ||
					cmpFunc(row->Strings[i], row->Strings[j]) < 0 && !ascending) {

					tmpCol->Assign(grid->Cols[i]);
					grid->Cols[i]->Assign(grid->Cols[j]);
					grid->Cols[j]->Assign(tmpCol);
				}

	} __finally {
		delete tmpCol;
	}
}
//--------------------------------------------------------------------------------------


bool __fastcall TMForm::SaveData(bool showDialog) {

	bool flag = false; // флаг дальнейшего сохранения // для выхода из внешнего цикла
	float d;
	for (int col = 1; col < Table->ColCount - 1; col++) {
		for (int row = 1; row < Table->RowCount - 1; row++) {
			if (!TryStrToFloat(Table->Cells[col][row], d) || d < 0) {
				if (Application->MessageBox((UnicodeString(L"Таблица содержит недопустимые символы или пустые ячейки!\nПри сохранении их содержимое будет проигнорировано.\nПродолжить сохранение?")).w_str(),
						Application->Title.w_str(), MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=ID_YES) {
					return false;

				} else {
					flag = true;
					break;
				}
			}
		}

		if (flag)
			break;
	}


	String sfname = SaveDialog1->FileName;
	//SaveDialog1->InitialDir = L"D:\\";
	SaveDialog1 -> Filter = L"Файлы АТП (*.atp) |*.atp; | Все файлы (*.*) |*.*;";
	if (showDialog) {
		if (!SaveDialog1->Execute())
			return false;

		sfname = SaveDialog1->FileName;
		int filep = sfname.LastDelimiter(".");
		if (filep)
			sfname = sfname.SubString(1,filep-1) ;
		SaveDialog1->FileName = sfname + L".atp";
	}

	try {
		//сохранение исходных данных в файл

		typedef float elemType;

		struct elem {
			elemType dist;
			elemType coeff;
		};

		DynamicArray<elem>  da;
		int c_num = Table->ColCount - 1;
		da.Length = c_num;

		float d;
		for (int i = 0; i < c_num; i++) {
			if (!TryStrToFloat(Table->Cells[i+1][1], d) || d < 0)
				da[i].dist = NaN;
			else
				da[i].dist = StrToFloat(Table->Cells[i+1][1]);

			if (!TryStrToFloat(Table->Cells[i+1][2], d) || d < 0)
				da[i].coeff = NaN;
			else
				da[i].coeff = StrToFloat(Table->Cells[i+1][2]);
		}

		fstream f;
		f.open(AnsiString(SaveDialog1->FileName).c_str(), std::ios_base::out | std::ios_base::binary);
		f.write((char*)&c_num, sizeof(c_num));
		f.write((char*)da.data(), c_num * sizeof(elem));
		f.close();

	} catch (Exception &e) {
		Application->MessageBox(Format(L"Ошибка сохранения файла \"%s\": \"%s\"",
									   ARRAYOFCONST((SaveDialog1->FileName, e.Message))).w_str(),
								Application->Title.w_str(), MB_OK|MB_ICONERROR);
		SaveDialog1->FileName = sfname;
		return false;
	}


    StatusBar->SimpleText = L"";
	mSave->Enabled = false;
	MemoForDataOutput -> Color =  clWindow;
	Caption = L"Расчет пробега - " + (SaveDialog1->FileName);
    TITLE = Caption;
	return true;
}


bool __fastcall TMForm::CheckChangesAndSave() {

	if (mSave -> Enabled) {

		String fname;
		if (!SaveDialog1->FileName.IsEmpty())
			fname = SaveDialog1->FileName;
		else
			fname = L"Безымянный";

		switch (Application->MessageBox((UnicodeString(L"Сохранить изменения в \"") + fname + L"\"?").w_str(),
				Application->Title.w_str(), MB_YESNOCANCEL|MB_ICONQUESTION)) {
			case ID_YES:
				if (!SaveData(SaveDialog1->FileName.IsEmpty()))
					return false;
				break;
			case ID_CANCEL:
				return false;
		}
	}
	return true;
}
//--------------------------------------------------------------------------------------

__fastcall TMForm::TMForm(TComponent* Owner)
	: TForm(Owner)
{

    Table -> Width =  (ClientWidth - Table -> Left * 2);

	Table -> Cells[0][0] = L"N АТП";
	Table -> Cells[0][1] = L"L, км";
	Table -> Cells[0][2] = L"b";

	MemoForDataOutput -> Lines -> Add(String(L"Среднее арифметическое b:        "));
	MemoForDataOutput -> Lines -> Add(String(L"Среднее арифметическое L:        "));
	MemoForDataOutput -> Lines -> Add(String(L"Среднеквадратичное отклонение L: "));
	MemoForDataOutput -> Lines -> Add(String(L"Среднеквадратичное отклонение b: "));
//  MemoForDataOutput -> Lines -> Add(String(L"Коэффициент ковариации:          ") + covLb);
	MemoForDataOutput -> Lines -> Add(String(L"Коэффициент корреляции:          "));

	Application->Title = TITLE;
	Table -> ColWidths[0] = Table->DefaultColWidth + 30;
	Table -> ColWidths[1] = -1;

    Table->Options=Table->Options >> goEditing; //
	Table->Options=Table->Options >> goColSizing; //

	//bSubCol->Enabled = false;
	bDelDiapCols->Enabled = false;
	mSave->Enabled = false;
	MemoForDataOutput -> Color =  clWindow;
	FSortedColNbr = 0;

	Caption = L"Расчет пробега - Безымянный";
	TITLE = L"Расчет пробега - Безымянный";
	Table->Font->OnChange = sgFontChanged;

}
//---------------------------------------------------------------------------



void __fastcall TMForm::mFileOpenClick(TObject *Sender) //Загрузить файл
{

	if (!CheckChangesAndSave())  //проверка на несохраненные изменения
		return;

	typedef float elemType;

	struct elem {
		elemType dist;
		elemType coeff;
	};

	DynamicArray<elem> da;

	int elCount, elCntBuf;//количество элементов / контрольное значение
	fstream f;


	StatusBar->SimpleText = L"";
	String f_name;
	OpenDialog1 -> Filter = L"Данные АТП (*.atp)|*.atp; |Bсе файлы| *.*; ";

	if(OpenDialog1->Execute())
		f_name = OpenDialog1 -> FileName;

	f.open(AnsiString(f_name).c_str(), std::ios_base::in | std::ios_base::binary);

	if (!f.is_open()) {
		StatusBar -> SimpleText  = UnicodeString(L" К сожалению, файл \"") + f_name + L"\" не удалось открыть";
		return;
	}

	f.seekg(0, ios::end);
	int n = f.tellg();

	if (n == 0) {
		f.close();
		StatusBar -> SimpleText = UnicodeString(L" Файл \"") + f_name + L"\" пуст";
		return;
	}

	bool flag = false;

	if ((n < sizeof(elCount)) || ((n - sizeof(elCount)) % sizeof(elemType))) {

		flag = true;

	}
	else {

		f.seekg(0);
		f.read((char*)&elCntBuf, sizeof(elCntBuf));

		if (elCntBuf != ((n - sizeof(elCntBuf)) / sizeof(elem))) {  //проверка elCount на четность  //elemType
			flag = true;
		}
	}

	if (flag) {
		f.close();
		StatusBar->SimpleText = UnicodeString(L" Файл \"") + f_name + L"\" поврежден!";

		Application->MessageBox((UnicodeString(L" Файл поврежден! ")).w_str(),
		Application->Title.w_str(), MB_OK|MB_ICONERROR);
		return;
	}

	TITLE = L"Расчет пробега - " + f_name;
	MForm->Caption = L"Расчет пробега - " + f_name;
	elCount = elCntBuf;


	da.Length = elCount;

	f.read((char*)da.data(), elCount * sizeof(elem));
	f.close();



	 for(int j = 0; j < Table->ColCount - 1; j++){
		Table -> Cells[j+1][0] = j+1;
		Table -> Cells[j+1][1] = L"";
		Table -> Cells[j+1][2] = L"";
	}

	Table -> ColCount = elCount + 1;
	Table -> ColWidths[1] = Table -> DefaultColWidth;

    Table->Options=Table->Options << goEditing; //
	Table->Options=Table->Options << goColSizing;//

	for(int j = 0; j < elCount; j++){
		Table -> Cells[j+1][0] = j+1;
		if (da[j].dist == da[j].dist)
			Table -> Cells[j+1][1] = FloatToStrF(da[j].dist,ffFixed, 7, 3);
		if (da[j].coeff == da[j].coeff)
			Table -> Cells[j+1][2] = FloatToStrF(da[j].coeff,ffFixed, 7, 3);
	}

	if (MemoForDataOutput -> Text != L"")
		MemoForDataOutput -> Color =  clInfoBk;


	if (mCellAutoWidth->Checked)
		AdjGridColWidths(Table);

	SaveDialog1->FileName = OpenDialog1 -> FileName;
	bDelDiapCols->Enabled = Table->ColWidths[1]!=-1;
	mSave->Enabled = false;
	FSortedColNbr = 0;
	MemoForDataOutput -> Color =  clWindow;
}
//---------------------------------------------------------------------------


void __fastcall TMForm::bCalcDrawClick(TObject *Sender) //Рассчитать
{

    StatusBar->SimpleText = L"";

	for (int col = 1; col < Table -> ColCount; col++) {
		for (int row = 1; row <= 2; row++) {


            if (Table -> Cells[col][row] == L"") {
				StatusBar -> SimpleText = L"Таблица не заполнена! Заполните пустые ячейки или удалите пустые столбцы!";
				Application->MessageBox((UnicodeString(L" Таблица не заполнена!\n Заполните пустые ячейки или удалите пустые столбцы!")).w_str(),
										Application->Title.w_str(), MB_OK|MB_ICONERROR);
				return;
			}

			double d;
			if (!TryStrToFloat(Table->Cells[col][row], d) || d < 0) {
				StatusBar -> SimpleText = UnicodeString(L"Недопустимая последовательность символов: \"" + Table -> Cells[col][row] + L"\" в ячейке [" + col + L"][" + row + L"]");
				Application->MessageBox((UnicodeString(L"Недопустимая последовательность символов: \"" + Table -> Cells[col][row] + L"\" в ячейке [" + col + L"][" + row + L"]")).w_str(),
										Application->Title.w_str(), MB_OK|MB_ICONERROR);
				return;
			}


		}
	}


	for (int i = 0 ; i < 5; i++)
		MemoForDataOutput -> Lines -> Strings[i] = MemoForDataOutput -> Lines -> Strings[i].Delete(34,39);

	float Lm = 0, bm = 0, Lk = 0, bk = 0, covLb = 0, cor = 0;

	try {


	for(int i = 1; i < Table -> ColCount; i++) {
		Lm += Table -> Cells[i][1].ToDouble();
		bm += Table -> Cells[i][2].ToDouble();
	}

	Lm /= Table -> ColCount - 1;
	bm /= Table -> ColCount - 1;

	for(int i = 1; i < Table -> ColCount; i++) {
		Lk += pow(Table -> Cells[i][1].ToDouble() - Lm, 2);
		bk += pow(Table -> Cells[i][2].ToDouble() - bm, 2);
		covLb += (Table -> Cells[i][1].ToDouble() - Lm) * (Table -> Cells[i][2].ToDouble() - bm);
	}


	Lk = sqrt(Lk /= Table -> ColCount - 1);
	bk = sqrt(bk /= Table -> ColCount - 1);
	covLb /= Table -> ColCount - 1;

	if (Lk * bk)     // if(Lk != 0 & bk != 0)
		cor = covLb / (Lk * bk);

	} catch(Exception &e) {

		Application->MessageBox(L"Введенное число слишком велико",
								Application->Title.w_str(), MB_OK|MB_ICONERROR);
        return;

	}

	MemoForDataOutput -> Color =  clWindow;
	MemoForDataOutput -> Lines -> Strings[0] += FloatToStrF(Lm,ffFixed, 7, 3);
	MemoForDataOutput -> Lines -> Strings[1] += FloatToStrF(bm,ffFixed, 7, 3);
	MemoForDataOutput -> Lines -> Strings[2] += FloatToStrF(Lk,ffFixed, 7, 3);
	MemoForDataOutput -> Lines -> Strings[3] += FloatToStrF(bk,ffFixed, 7, 3);
//  MemoForDataOutput -> Lines -> Add(covLb);
	MemoForDataOutput -> Lines -> Strings[4] += FloatToStrF(cor,ffFixed, 7, 3);


	Series1->Clear();
	Series2->Clear();



	Series2->Marks->Color = clActiveCaption;
	Series2->Marks->Style=smsValue;
	Series1->Marks->Style=smsValue;

	double y, x;
	for(int i = 0; i < Table -> ColCount-1; i++){
		y = Table-> Cells[i+1][1].ToDouble();
		x = i+1;
		Series1->Add(y,x,clBlue);
		y = Table-> Cells[i+1][2].ToDouble();
		Series2->Add(y,x,clGreen);
	}

}
//--------------------------------------------------------------------------



void __fastcall TMForm::bAddColClick(TObject *Sender) // добавление столбца
{

	if(Table->ColWidths[1] == -1) {
		Table->ColWidths[1]=Table->DefaultColWidth;

		Table->Options=Table->Options << goEditing; //
		Table->Options=Table->Options << goColSizing;//

		Table->Cells[Table->ColCount - 1][0] = Table->ColCount - 1 ;
		if (!cbSelDiap->Checked)
			Table->Options = Table->Options << goEditing;
	} else
		Table->ColCount++;

	Table->Cells[Table->ColCount - 1][0] = Table->ColCount - 1 ;
	Table -> Cells[Table -> ColCount][1] = L"";
	Table -> Cells[Table -> ColCount][2] = L"";

	Table->Col = Table->ColCount - 1;
	Table->SetFocus();


	mSave->Enabled = true;
	bDelDiapCols->Enabled = true;
	MemoForDataOutput -> Color =  clInfoBk;
    StatusBar->SimpleText = L"";
	Caption = String(TITLE) + L"*";
}
//---------------------------------------------------------------------------


void __fastcall TMForm::FResize(TObject *Sender) //
{
	//перерасчет размеров таблицы и TChart при изменении размеров окна

	ChartForDist -> Width = (ClientWidth - ChartForDist -> Left * 1.5);
	//ChartForDist -> Height = (ClientHeight - ChartForDist -> Top * 1.1);
	ChartForDist -> Height = (ClientHeight - ChartForDist -> Top - StatusBar->Height - 5);

	//bDataProcBtn->Left = ClientWidth - ChartForDist -> Left * 6;
	bDataProcBtn->Left = ClientWidth - ChartForDist -> Left - bDataProcBtn->Width;
	Table -> Width =  (ClientWidth - Table -> Left * 1.5);
	MemoForDataOutput -> Width = (ClientWidth - MemoForDataOutput -> Left * 2);
}
//---------------------------------------------------------------------------


void __fastcall TMForm::TableKeyPress(TObject *Sender, System::WideChar &Key)
{
	if (!Table->EditorMode) {
		return;
	}

	if (Key==VK_ESCAPE) {
		if (Table->EditorMode) {
			Table->Cells[Table->Col][Table->Row] = FCurCellText;
			if (Table->Options.Contains(goAlwaysShowEditor)) {
				Table->EditorMode = false;
			}
		}
	}


	if (! (Key >= L'0' && Key <= L'9' || Key == FormatSettings.DecimalSeparator ||
		Key == L'+' || Key == L'-' || Key == L'e' || Key == L'E') &&
		Key != VK_BACK && Key != 3 && Key != 22 && Key != 24 && Key != 26 && Key != 27 && Key != 13 && Key != 9 && Key != 9){
			StatusBar -> SimpleText = L"Недопустимый символ \""+UnicodeString(Key)+L"\"";
			Key = 0;
	} else StatusBar -> SimpleText = L"";


}
//---------------------------------------------------------------------------


bool __fastcall TMForm::CheckGridCell(int col, int row) {
	float d;
	if (!TryStrToFloat(Table->Cells[col][row], d) || d < 0) {
		if(Table->Cells[col][row]!=L"") {//Чтобы не ругался на Ctrl+Z, Enter и т.д

			return false;
		}

	}
	return  true;
}

void __fastcall TMForm::TableSetEditText(TObject *Sender, int ACol, int ARow, const UnicodeString Value) //Ввод данных в таблицу
{
	StatusBar -> SimpleText = L"";

	if (!((TStringGrid*)Sender)->Tag) {
		if(Table->EditorMode)
			if(!CheckGridCell(ACol, ARow)) {
				StatusBar -> SimpleText = UnicodeString(L"Некорректная последовательность символов: \"" +
				Table -> Cells[ACol][ARow] + L"\" в ячейке [" + ACol + L"][" + ARow + L"]");
				//Application ->MessageBox(StatusBar->SimpleText.w_str(),
				//		(Application->Title).w_str(), MB_OK|MB_ICONERROR);
			}
	}
	else
		((TStringGrid*)Sender)->Tag=false;


	if (mCellAutoWidth->Checked)
		Table->ColWidths[ACol]=CalcGridColWidth(Table,ACol);

	if (!mSave->Enabled && !Table->EditorMode && FCurCellText != Value) {
		mSave->Enabled = true;
		MemoForDataOutput -> Color = clInfoBk;
		Caption = String(TITLE) + L"*";
	}
}
//---------------------------------------------------------------------------



void __fastcall TMForm::mResSaveAsClick(TObject *Sender) //Сохранить
{

	SaveDialog1 -> Filter = L"Файлы с вычисленными коэффициентами (*.txt) |*.txt; | Все файлы (*.*)";
	//SaveDialog1->InitialDir = L"D:\\tempKP\\";
	SaveDialog1->FileName = L"";
	if (SaveDialog1->Execute()) {

		String sfname = SaveDialog1->FileName;
		int filep = sfname.LastDelimiter(".");
		if (filep)
			sfname = sfname.SubString(1,filep-1) ;
		SaveDialog1->FileName = sfname + L".txt";

		//Сохранение содержимого табл. редактора
		MForm->MemoForDataOutput -> Lines -> SaveToFile(SaveDialog1->FileName);

	}

	  SaveDialog1->FileName=OpenDialog1->FileName;
      StatusBar->SimpleText = L"";
}
//---------------------------------------------------------------------------


void __fastcall TMForm::cbSelDiapClick(TObject *Sender) //режим выделения ячеек
{
	if (cbSelDiap->Checked)
		Table->Options >> goEditing;
	else
		Table->Options << goEditing;
}
//---------------------------------------------------------------------------


void __fastcall TMForm::bDelDiapColsClick(TObject *Sender)
{
	if (Table->ColWidths[1]==-1)
		return;

	if (Application->MessageBox((UnicodeString(L"Вы действительно хотите удалить выделенные составы (") +
		Table->Selection.Left + L'-' +
		Table->Selection.Right + L")?").w_str(),
			Application->Title.w_str(), MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=ID_YES)
		return;



	int n = Table->Selection.Right - Table->Selection.Left + 1;

	if (Table->ColCount - n >= 2) {

		if (Table->Selection.Left < Table->ColCount - 1)
			for (int i=Table->Selection.Right + 1; i < Table->ColCount; i++)
				Table->Cols[i-n]=Table->Cols[i];

		Table->ColCount=Table->ColCount-n;
		for(int j = 0; j < Table->ColCount; j++)
			Table->Cells[j+1][0] = j+1;
	}
	else {
		Table->ColCount = 2;
		Table->ColWidths[1] = -1;

		Table->Options=Table->Options >> goEditing; //
		Table->Options=Table->Options >> goColSizing;//

		Table->Cells[1][1]=L"";
		Table->Cells[1][2]=L"";



		bDelDiapCols->Enabled = false;

	}


    StatusBar->SimpleText = L"";
	mSave -> Enabled = true;
	MemoForDataOutput->Color =  clInfoBk;
	Caption = String(TITLE) + L"*";
}
//---------------------------------------------------------------------------

void __fastcall TMForm::bInsColClick(TObject *Sender)
{
	if (Table->ColWidths[1]==-1) {
		Table->ColWidths[1]=Table->DefaultColWidth;

        Table->Options=Table->Options << goEditing; //
		Table->Options=Table->Options << goColSizing;//

		cbSelDiapClick(NULL);
	}
	else{
		for (int i = Table->ColCount - 1; i >= Table->Col; i--)
			Table->Cols[i+1]=Table->Cols[i];
		Table->ColCount++;
	}
	Table->Cols[Table->Col]->Clear();

	for(int j = 0; j < Table->ColCount; j++)
		Table -> Cells[j+1][0] = j+1;

    StatusBar->SimpleText = L"";
	bDelDiapCols->Enabled = true;
	mSave->Enabled = true;
	MemoForDataOutput -> Color =  clInfoBk;
	Caption = String(TITLE) + L"*";
}
//---------------------------------------------------------------------------

void __fastcall TMForm::TableSelectCell(TObject *Sender, int ACol, int ARow, bool &CanSelect)
{

	StatusBar->SimpleText = L"";

	if (!mSave->Enabled && Table->EditorMode && FCurCellText != Table->Cells[Table->Col][Table->Row]){
		mSave->Enabled = true;
		Caption = String(TITLE) + L"*";
	}


		if(!CheckGridCell(ACol, ARow)) {
			StatusBar -> SimpleText = UnicodeString(L"Некорректная последовательность символов: \"" +
			Table -> Cells[ACol][ARow] + L"\" в ячейке [" + ACol + L"][" + ARow + L"]");
				//Application ->MessageBox(StatusBar->SimpleText.w_str(),
				//		(Application->Title).w_str(), MB_OK|MB_ICONERROR);
		}

	if (Table->EditorMode && !CheckGridCell(Table->Col, Table->Row)) {
		StatusBar -> SimpleText = UnicodeString(L"Некорректная последовательность символов: \"" +
				Table -> Cells[Table->Col][Table->Row] + L"\" в ячейке [" + Table->Col + L"][" + Table->Row + L"]");

		//Application ->MessageBox(StatusBar->SimpleText.w_str(),
		//(Application->Title).w_str(), MB_OK|MB_ICONERROR);

		Table->Tag = true;
	}

}
//---------------------------------------------------------------------------

void __fastcall TMForm::mCellAutoWidthClick(TObject *Sender)
{
	if (mCellAutoWidth->Checked)
		AdjGridColWidths(Table);
	else
		for (int i = 0; i < Table->ColCount; i++)
			Table->ColWidths[i] = Table->DefaultColWidth;

    StatusBar->SimpleText = L"";
}
//---------------------------------------------------------------------------

void __fastcall TMForm::TableGetEditText(TObject *Sender, int ACol, int ARow, UnicodeString &Value)
{
	FCurCellText = Value;
}
//---------------------------------------------------------------------------


 double cmpFunc2 (const UnicodeString &s1, const UnicodeString &s2) {
	return StrToFloat(s1) - StrToFloat(s2);
}
//---------------------------------------------------------------------------


void __fastcall TMForm::TableDblClick(TObject *Sender)
{
	TPoint p;
	GetCursorPos(&p);
	p=Table->ScreenToClient(p);

	if (GetCursor() == Screen->Cursors[crHSplit]) {

		TGridCoord gc = Table->MouseCoord(p.X-4,p.Y);
		int w = CalcGridColWidth(Table,gc.X);
		if (Table->ColWidths[gc.X] != w)
			Table->ColWidths[gc.X] = w;
		else
			Table->ColWidths[gc.X] = MIN_COL_WIDTH;
	}
	else 
	{
		TGridCoord gc = Table->MouseCoord(p.X, p.Y);
		if (gc.X == 0) {
			int i = abs(FSortedColNbr) - 1;

			((TMyStringGrid*)Sender)->InvalidateCell(i, 0);


			if (FSortedColNbr && i==gc.Y)
				FSortedColNbr = -FSortedColNbr;
			else
				FSortedColNbr = gc.Y + 1;
				
			i = gc.Y;

			Table->Cols[Table->Col]->Objects[0]=(TObject*)true;

			SortGridByRow(Table, i, FSortedColNbr > 0, cmpFunc2);

			for (i = 1; i < Table->ColCount; i++)
				if (Table->Cols[i]->Objects[0]) {
					Table->Cols[i]->Objects[0] = (TObject*)false;
					Table->Col = i;
					break;
				}

			if (mCellAutoWidth->Checked)
				AdjGridColWidths(Table);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TMForm::TableDrawCell(TObject *Sender, int ACol, int ARow, TRect &Rect,
		  TGridDrawState State)
{

    float d;
	if ((!TryStrToFloat(Table->Cells[ACol][ARow], d) || d < 0) && ACol > 0) {
		if(Table->Cells[ACol][ARow]==L"") {//Чтобы не ругался на Ctrl+Z, Enter и т.д

			Table->Canvas->Brush->Color = clInfoBk;
			TRect r = Rect;
			Table->Canvas->TextRect(r,r.Left+6,r.Top+2,Table->Cells[ACol][ARow]);
		}
		else {
			Table->Canvas->Brush->Color = (TColor)0x008080FF;
			TRect r = Rect;
			Table->Canvas->TextRect(r,r.Left+6,r.Top+2,Table->Cells[ACol][ARow]);
		}
	}


	if (State.Contains(gdSelected))
	{
		Table->Canvas->Brush->Color = (TColor)0x00FF8000;//0xBAFDFD;// выбираем любой цвет
		Table->Canvas->FillRect(Rect);
		Table->Canvas->Font->Color = clWhite;
		TRect r = Rect;
		Table->Canvas->TextRect(r,r.Left+6,r.Top+2,Table->Cells[ACol][ARow]);
	}


	if (ACol != 0 || !FSortedColNbr || abs(FSortedColNbr)-1 != ARow )
		return;
	Table->Canvas->Font=Table->Font;
	Table->Canvas->Font->Size+=2;
	Table->Canvas->Brush->Style = bsSolid;
	TSize ts = Table->Canvas->TextExtent(L"◄");

	Table->Canvas->TextOut(Rect.Right - ts.Width - 2, Rect.Top + (Rect.Height() - ts.Height) / 2,
							FSortedColNbr > 0 ? L"◄" : L"►");
}
//---------------------------------------------------------------------------

void __fastcall TMForm::CloseQuery(TObject *Sender, bool &CanClose)
{
	if (!CheckChangesAndSave())
		CanClose = false;
}
//---------------------------------------------------------------------------


void __fastcall selectFont(TFont* font, const TFontDialogOptions &options = TFontDialogOptions() << fdEffects) {

	class THelper {
		public:
			TFont *srcFontRef;
			bool srcFontAppliedTo, dlgFontChanged;
			void __fastcall ApplyBtnClick(TObject *Sender, HWND Wnd) {
				if (dlgFontChanged) {
					srcFontRef->Assign(((TFontDialog*)Sender)->Font);
					srcFontAppliedTo = true;
					dlgFontChanged = false;
				}
			}
			void __fastcall DlgFontChange(TObject *Sender) {
				dlgFontChanged = true;
			}
			THelper(TFont *_font) {
				srcFontRef = _font;
				srcFontAppliedTo = false;
				dlgFontChanged = false;
            }
	}
		hlpr(font);

	TFontDialog *fontDlg = new TFontDialog(NULL);
	try {
		fontDlg->Options=options;
		fontDlg->OnApply=hlpr.ApplyBtnClick;
		fontDlg->Font=font;
		fontDlg->Font->OnChange = hlpr.DlgFontChange;
		fontDlg->MinFontSize = 6;
        fontDlg->MaxFontSize = 18;
		TFont *svdFont = new TFont();
		try {
			svdFont->Assign(font);
			if (!fontDlg->Execute()) {
				if (hlpr.srcFontAppliedTo)
					font->Assign(svdFont);
				return;
			}
			if (hlpr.dlgFontChanged)
				font->Assign(fontDlg->Font);

		} __finally {
			delete svdFont;
		}
	} __finally {
		delete fontDlg;
	}
}
//---------------------------------------------------------------------------

void __fastcall TMForm::sgFontChanged(System::TObject* Sender) {

	//Корректировка высоты столбцов таблицы при изменении шрифта
	Table->Canvas->Font->Assign(Table->Font);
	Table->DefaultRowHeight = Table->Canvas->TextHeight(L"hy")+4;
	if (mCellAutoWidth->Checked)
		AdjGridColWidths(Table);
}

void __fastcall TMForm::mFontSettingsClick(TObject *Sender)
{
	selectFont(Font);
    StatusBar->SimpleText = L"";
}
//---------------------------------------------------------------------------


void __fastcall TMForm::mCreateClick(TObject *Sender)
{
	if (!CheckChangesAndSave())
		return;

	Table->ColCount = 2;
	Table->ColWidths[1] = -1;

	Table->Options=Table->Options >> goEditing; //
	Table->Options=Table->Options >> goColSizing; //

	//Table->Options >> goEditing;
    Table->Cells[1][1]=L"";
	Table->Cells[1][2]=L"";


	SaveDialog1->FileName = L"";
	StatusBar->SimpleText = L"";
	mSave->Enabled=false;
	Caption = L"Расчет пробега - Безымянный";

}
//---------------------------------------------------------------------------


void __fastcall TMForm::mSaveClick(TObject *Sender)  //Сохранить
{
	SaveData(SaveDialog1->FileName.IsEmpty());
}
//---------------------------------------------------------------------------


void __fastcall TMForm::mSaveAsClick(TObject *Sender)  //Сохранить как...
{
	SaveData(true);
}
//---------------------------------------------------------------------------

void __fastcall TMForm::FontDialog1Apply(TObject *Sender, HWND Wnd)
{
	((TFont*)FontDialog1->Tag)->Assign(FontDialog1->Font);
}
//---------------------------------------------------------------------------


void __fastcall TMForm::mExitClick(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------

void __fastcall TMForm::BMPClick(TObject *Sender)
{

	String fExt = AnsiLowerCase(ExtractFileExt(SaveDialog1->FileName));
	if (fExt == L"")
		fExt = L".bmp";
	else
		fExt = L"";
	ChartForDist->SaveToBitmapFile(UnicodeString(SaveDialog1 -> FileName)+fExt); //L".bmp");

	SaveDialog1->FileName=OpenDialog1->FileName;

}
//---------------------------------------------------------------------------

void __fastcall TMForm::JPEGClick(TObject *Sender)
{

	TCanvas *Source = new TCanvas();
	Source->Handle = GetDC(ChartForDist->Handle);
	Graphics::TBitmap *bmp = new Graphics::TBitmap();
	bmp->SetSize(ChartForDist->ClientWidth, ChartForDist->ClientHeight);
	RECT src = ChartForDist->ClientRect;
	RECT dst = {0, 0, ChartForDist->ClientWidth, ChartForDist->ClientHeight};
	bmp->Canvas->CopyRect(dst, Source, src);

	TJPEGImage *jpeg = new TJPEGImage();
	jpeg->Assign(bmp);

	//Проверка на наличие расширения
	String fExt = AnsiLowerCase(ExtractFileExt(SaveDialog1->FileName));
	if (fExt == L"")
		fExt = L".jpeg";
	else
		fExt = L"";

	jpeg->SaveToFile(UnicodeString(SaveDialog1 -> FileName) + fExt);
	jpeg->Free();
	bmp->Free();
	ReleaseDC(ChartForDist->Handle, Source->Handle);
	Source->Free();

	SaveDialog1->FileName=OpenDialog1->FileName;
}
//---------------------------------------------------------------------------

void __fastcall TMForm::GIFClick(TObject *Sender)
{

	TCanvas *Source = new TCanvas();
	Source->Handle = GetDC(ChartForDist->Handle);
	Graphics::TBitmap *bmp = new Graphics::TBitmap();
	bmp->SetSize(ChartForDist->ClientWidth, ChartForDist->ClientHeight);
	RECT src = ChartForDist->ClientRect;
	RECT dst = {0, 0, ChartForDist->ClientWidth, ChartForDist->ClientHeight};
	bmp->Canvas->CopyRect(dst, Source, src);

	TGIFImage* gif= new TGIFImage();
	gif->Assign(bmp);

	//Проверка на наличие расширения
	String fExt = AnsiLowerCase(ExtractFileExt(SaveDialog1->FileName));
	if (fExt == L"")
		fExt = L".gif";
	else
		fExt = L"";

	gif->SaveToFile(UnicodeString(SaveDialog1 -> FileName) + fExt);
	gif->Free();
	bmp->Free();
	ReleaseDC(ChartForDist->Handle, Source->Handle);
	Source->Free();

	SaveDialog1->FileName=OpenDialog1->FileName;

}
//---------------------------------------------------------------------------


void __fastcall TMForm::OpenDialog1Show(TObject *Sender)
{
//
}
//---------------------------------------------------------------------------


void __fastcall TMForm::mSaveImgAsClick(TObject *Sender)
{

    StatusBar->SimpleText = L"";
   //	SaveDialog1->InitialDir = L"D:\\tempKP\\";
	SaveDialog1->FileName = L"";
	MForm->SaveDialog1 -> Filter = L"Графики в формате (*.bmp)|*.bmp;|Графики в формате (*.jpeg)|*.jpeg;|Графики в формате (*.gif)|;*.gif; |Bсе файлы (*.*)| *.*;";
	if (!SaveDialog1->Execute())
		return;
	SaveDialog1->FileName=OpenDialog1->FileName;      //
	//Введенное пользователем имя файла обрабатывается в обработчике  TMForm::SaveDialog1CanClose(...)
}

//---------------------------------------------------------------------------

void __fastcall TMForm::SaveDialog1CanClose(TObject *Sender, bool &CanClose)
{

	//Для предотвращения ошибок при сохранении изображений
	String str = SaveDialog1->Filter;  // str - расширение извлеченное из фильтра
	int filep1 = str.Pos("(");
	int filep2 = str.Pos(")");
	str = str.SubString(filep1+2,filep2-2-filep1) ;

	String FileExt = AnsiLowerCase(ExtractFileExt(SaveDialog1->FileName));
	if (FileExt == L"") //при отсутствии заданного расширения оно выбирается по фильтру
		switch (SaveDialog1->FilterIndex) {
			case 1:  //bmp
				ChartForDist->SaveToBitmapFile(UnicodeString(SaveDialog1 -> FileName) + L".bmp");
				break;

			case 2:  //jpeg
				TMForm::JPEGClick(NULL);
				break;

			case 3:  //gif
				TMForm::GIFClick(NULL);
				break;

			case 4:  //* по умолочанию сохраняет в bmp, если выбран фильтр "Все файлы (*.*)"
				 ChartForDist->SaveToBitmapFile(UnicodeString(SaveDialog1 -> FileName) + L".bmp");

		} //проверка на допустимое расширение
		else if (FileExt == L".bmp")
			TMForm::BMPClick(NULL);
		else if (FileExt == L".jpeg")
			TMForm::JPEGClick(NULL);
		else if (FileExt == L".gif")
			TMForm::GIFClick(NULL);
		else if ((FileExt == L".atp") || str == L".atp")   // str - расширение извлеченное из фильтра //Расширение введенное пользователем не рассматривается, отсекается и устанавливается нужное  //&&
			CanClose = true;
		else if ((FileExt == L".txt") || str == L".txt")   //Расширение введенное пользователем не рассматривается  //&&
			CanClose = true;
		else
			if (Application->MessageBox((UnicodeString(L"Приложение не предусматривает сохранение в данном формате.\n")).w_str(),
				Application->Title.w_str(), MB_OK|MB_ICONERROR)) {
					CanClose = false;
				}


}
//---------------------------------------------------------------------------

void __fastcall TMForm::SaveDialog1Show(TObject *Sender)
{
//
}
//---------------------------------------------------------------------------

