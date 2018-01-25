#include "s2f.h"
#include "ui_s2f.h"
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDirIterator>

S2F::S2F(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::S2F)
{
    ui->setupUi(this);
    ui->koeffkalEdit->setValidator(new QDoubleValidator());
    ui->koeffEdit->setValidator(new QDoubleValidator());
    ui->lEdit->setValidator(new QDoubleValidator());
    ui->sEdit->setValidator(new QDoubleValidator());
    ui->vEdit->setValidator(new QDoubleValidator());
    ui->procentEdit->setValidator(new QDoubleValidator());
}

S2F::~S2F()
{
    delete ui;
}

/*void S2F::ReadSPM0(QDataStream &data)
{

}*/


char dataRaw[32][524288];//Place for raw data of fields from spm1 files. 2 bytes for each point. Max 256*256 points.
quint16 dataShort[32][262144];//Same data but transformed into 16 bit integers.
float dataMuliplied[32][262144];//Data after applying z-multiplier.
//32 is borderline amount of fields for that file format. The service info won't support more than 32.


void S2F::ReadSPM1(QDataStream &data)
{
    char HEAD_SPM1[223]; //Getting HEAD_SPM1 from file.
    data.readRawData(HEAD_SPM1,223);
    quint16 MaxNx, MaxNy, Nx[32], Ny[32], field_amount=0;
    quint16 what_this;
    memcpy(&MaxNx,HEAD_SPM1+49,2);
    memcpy(&MaxNy,HEAD_SPM1+51,2);
    memcpy(&what_this,HEAD_SPM1+59,2);
    while(what_this)//Subfield what_this contains info about stored fields witch we can use to find amount of those.
    {
        field_amount++;
        what_this &= (what_this-1); //current AND itself-1 to remove the least bit. repeat untill its 0.
    }
    char notifications[32][336];//Getting all notifications about stored fields of data.
    for(int i=0;i<field_amount;i++)
        data.readRawData(notifications[i], 336);

    char fieldName[32][32], scaleXYname[32][6], scaleZname[32][6];//info from notifications about fields
    float scaleX[32], scaleY[32], scaleZ[32];
    for(int i=0; i<field_amount;i++)//Getting that information.
    {
        strncpy(fieldName[i], notifications[i], 32);
        strncpy(scaleXYname[i], notifications[i]+68, 6);
        strncpy(scaleZname[i], notifications[i]+74, 6);
        memcpy(&Nx[i], notifications[i]+34, 2);
        memcpy(&Ny[i], notifications[i]+36, 2);
        memcpy(&scaleX[i], notifications[i]+40, 4);
        memcpy(&scaleY[i], notifications[i]+44, 4);
        memcpy(&scaleZ[i], notifications[i]+48, 4);
    }


    //Getting all raw data
    for(int i=0; i<field_amount;i++)
        data.readRawData(dataRaw[i], Nx[i]*Ny[i]*2);
    for(int i=0; i<field_amount;i++)//Trasform raw data into 16bit integers.
    {
        for(int j=0; j<(Nx[i]*Ny[i]);j++)
        {
            memcpy(&dataShort[i][j], dataRaw[i]+(2*j), 2);
        }
    }
    for(int i=0; i<field_amount;i++)//Multiping data by z-multiplier
    {
        for(int j=0; j<(Nx[i]*Ny[i]);j++)
        {
            dataMuliplied[i][j]=dataShort[i][j]*scaleZ[i];
        }
    }


    for(int i=0;i<field_amount;i++)//Creation and geneation of .txt files from .spm fields
    {
        int extPointPos = InputFileName.lastIndexOf(".");//Generating filenames based on .spm filename
        OutputFileName = InputFileName.left(extPointPos) + "#_" + QString::fromLatin1(fieldName[i]) + "_txt.txt";
        QFile out(OutputFileName);
        if(!out.open(QIODevice::WriteOnly))
            return;
        QTextStream outText(&out);
        //Generating .txt files from fields
        outText.setFieldWidth(16);
        outText.setFieldAlignment(QTextStream::AlignLeft);
        float currX=0, currY=0;
        outText << "Name: " + QString::fromLocal8Bit(fieldName[i]);//Name of the field
        outText << "\nNunColumns: " + QString::number(Nx[i]);//Amount of the columns
        outText << "\nNunRows: " + QString::number(Ny[i]);//Amount of the rows
        outText << "\nX, " + QString::fromLocal8Bit(scaleXYname[i]) << " Y, " + QString::fromLocal8Bit(scaleXYname[i])
                << " Z, " + QString::fromLocal8Bit(scaleZname[i]);//Axis and their dimensions.
        for(int j = 0; j<Nx[i]*Ny[i]; j++)//The data itself.
        {
            outText << qPrintable("\n" + QString::number((currX*scaleX[i]), 'f', 3)) << qPrintable(" " + QString::number((currY*scaleY[i]), 'f', 3))
                    << qPrintable(" " + QString::number(dataMuliplied[i][j], 'f', 3));
            if(currX==MaxNx-1)
            {
                currX=0;
                currY++;
            }else currX++;
        }
    }
    QMessageBox confirmation;
    confirmation.setText("Перевод завершен. Файлы-поля могут быть найдены в папке с исходным файлом.");
    confirmation.exec();
}

/*void S2F::ReadMPS(QDataStream &data)
{

}*/

void S2F::ReadSpm()
{
    if(InputFileName=="")//Default inputfilename
    {
        QMessageBox MTname;
        MTname.setText("Не выбран .spm файл. Пожалуйста, выберите.");
        MTname.exec();
        InputFileName = QFileDialog::getOpenFileName(this, tr("Открыть .spm файл"),"/", tr("Spm files (*.spm)"));
        ui->lineEditIn->setText(InputFileName);
        return;
    }
    QFile spm(InputFileName);//Opening input file
    if(!spm.open(QIODevice::ReadOnly))
        return;
    QDataStream data(&spm);
    qint8 spmType; data>>spmType;//Getting filetype.
    QMessageBox unknownFormatBox;
    switch(spmType)//Deciding what filetype is opened and callig for respective function.
    {
        /*case 0:
            ReadSPM0(data);
            break;*/
        case 1:
            ReadSPM1(data);
            break;
        /*case 2:
            ReadMPS(data);
            break;*/
        default:
            unknownFormatBox.setText("Unknown file format.");
            unknownFormatBox.setIcon(QMessageBox::Critical);
            unknownFormatBox.exec();
            break;
    }

}

short getFloatFromString(const QString &String)
{
    QRegExp rx("\\d+");
    rx.indexIn(String);
    QStringList list = rx.capturedTexts();
    return list.begin()->toShort();
}

float find_delta_field(float arr[], int size, short numR)
{
    float DELTA[256], delta=0, p = 0, deltaField = 0;
    int k=0, c=numR-1, g=0;
    while(k<size)
    {
        delta += arr[k];
        if(k==c)
        {
            delta /= numR;
            DELTA[g] = delta;
            g++;
            c += numR;
            delta=0;
        }
        k++;
    }
    k = 0; p =0;
    int G = 0;
    while(G<g)
    {
        p = DELTA[G];
        deltaField += p;
        if(p != 0)
            k++;
        G++;
    }
    deltaField /= k;
    return deltaField;
}

void S2F::CalculateFriction()
{
    if(InputFileNameFw=="" || InputFileNameBw=="")//Default inputfilenames
    {
        QMessageBox MTname;
        MTname.setText("Не выбраны .txt файлы. Пожалуйста, выберите.");
        MTname.exec();
        InputFileNameFw = QFileDialog::getOpenFileName(this, tr("Открыть .txt файл прямого хода"),"/", tr("txt files (*torsion_*.txt)"));
        ui->lineEditFw->setText(InputFileNameFw);
        InputFileNameBw = QFileDialog::getOpenFileName(this, tr("Открыть .txt файл обратного хода"),"/", tr("txt files (*torsion*bw*.txt)"));
        ui->lineEditBw->setText(InputFileNameBw);
        return;
    }
    QFile txtFw(InputFileNameFw), txtBw(InputFileNameBw);//Opening input files
    if(!txtFw.open(QIODevice::ReadOnly) || !txtBw.open(QIODevice::ReadOnly))
        return;


    QTextStream FwStream(&txtFw), BwStream(&txtBw);
    QString FC, FR, BC, BR;
    FwStream.readLineInto(NULL);//read fieldname into null
    FwStream.readLineInto(&FC);//read column count into qstring
    FwStream.readLineInto(&FR);//read row count into qstring
    FwStream.readLineInto(NULL);//read axis names and their dimensions into null.
    BwStream.readLineInto(NULL);//repeat for backwards file
    BwStream.readLineInto(&BC);
    BwStream.readLineInto(&BR);
    BwStream.readLineInto(NULL);
    short FwC = getFloatFromString(FC), //Column count in forward
    FwR = getFloatFromString(FR), //rows
    BwC = getFloatFromString(BC), //Column count in backward
    BwR = getFloatFromString(BR); //rows


    QStringList FData, BData;
    for(int i=0; i<FwC*FwR; i++)
    {
        FData.append(FwStream.readLine());//We read all data into list of strings for both files.
    }
    for(int i=0; i<BwC*BwR; i++)
    {
        BData.append(BwStream.readLine());
    }


    QRegExp rx("\\d+\\.\\d+");//primitive regular expression to find floats in strings. 1st is x coordinate, 2nd - y, 3rd - z.
    float FwZ[65536], BwZ[65536];//As we need only z, we allocate memory only for them.
    int FwZsize = FwC*FwR, BwZsize = BwC*BwR;//Total amount of data rows.
    for(int i=0;i<FwC*FwR;i++)//get z-axis values from forward file
    {
        int posF = 0;
        QStringList listF;
        while ((posF = rx.indexIn(FData[i], posF)) != -1)
        {
                posF+= rx.matchedLength();
                listF.append(rx.capturedTexts());
        }
        FwZ[i] = listF[2].toFloat();
    }
    for(int i=0;i<BwC*BwR;i++)//and from backward file
    {
        int posB = 0;
        QStringList listB;
        while ((posB = rx.indexIn(BData[i], posB)) != -1)
        {
                posB+= rx.matchedLength();
                listB.append(rx.capturedTexts());
        }
        BwZ[i] = listB[2].toFloat();
    }

    //Calculate delta of fields.
    float deltaFieldF = find_delta_field(FwZ, FwZsize, FwR), deltaFieldB = find_delta_field(BwZ, BwZsize, BwR);
    float DZ = abs(deltaFieldF - deltaFieldB);//Main variable for friction coeff. calculation.

    double ktr=(2*DZ*l)/(3*(1+v)*(65535*procent)*s*2); // friction coeff.
    double Zreal=DZ/koeffkolibrovki;
    double Zn=(65535)/koeffkolibrovki;
    double Ffr=Zreal*l*k/(3*(1+v)*s);
    //double Ftr=((DZ/2)*l*k)/(3*s*(1+v));
    double Fn=k*Zn;


    int extPointPos = InputFileNameFw.lastIndexOf(".");//Generating filenames based on input filename
    OutputFileName = InputFileNameFw.left(extPointPos) + "_friction_txt.txt";
    QFile out(OutputFileName);
    if(!out.open(QIODevice::WriteOnly))
        return;
    QTextStream outText(&out);
    //Generating .txt files
    outText << InputFileNameFw + ": Расчет завершен успешно.\n" +  "Ktr: " + QString::number(ktr)
               + "; Ffr: " + QString::number(Ffr) + "; Fn: " + QString::number(Fn)
               +  "\nКонстанты:\n" +  "Длина кантеливера, м\t" + QString::number(l) +  "\tКоэффициент жесткости, Н/м\t" + QString::number(k)
               +  "\tВысота иглы, м\t" + QString::number(s) +  "\tКоэффициент Пуассона\t" + QString::number(v) +  "\tНагрузка, %/100\t" + QString::number(procent)
               +  "\tКалибровачный коэффициент\t" + QString::number(koeffkolibrovki) + "\n\n";

    QMessageBox confirmation;
    confirmation.setText("Расчет завершен. Файл с результатами могут быть найдены в папке с исходным файлом.");
    confirmation.exec();
}

QString S2F::spm2friction()
{
    if(InputFileName=="")//Default inputfilename
    {
        QMessageBox MTname;
        MTname.setText("Не выбран .spm файл. Пожалуйста, выберите.");
        MTname.exec();
        InputFileName = QFileDialog::getOpenFileName(this, tr("Открыть .spm файл"),"/", tr("Spm files (*.spm)"));
        ui->lineEditIn_2->setText(InputFileName);
        return "";
    }
    QFile spm(InputFileName);//Opening input file
    if(!spm.open(QIODevice::ReadOnly))
        return InputFileName + ": Ошибка открытия файла\n";
    QDataStream data(&spm);
    qint8 spmType; data>>spmType;//Getting filetype.
    switch(spmType)//Deciding what filetype is opened and callig for respective function.
    {
        /*case 0:
            ReadSPM0(data);
            break;*/
        case 1:
            break;
        /*case 2:
            ReadMPS(data);
            break;*/
        default:
            return InputFileName + ": Ошибка: неподдерживаемый формат. Только формат spm1 поддерживается.\n";
    }


    char HEAD_SPM1[223]; //Getting HEAD_SPM1 from file.
    data.readRawData(HEAD_SPM1,223);
    quint16 MaxNx, MaxNy, Nx[32], Ny[32], field_amount=0;
    quint16 what_this;
    memcpy(&MaxNx,HEAD_SPM1+49,2);
    memcpy(&MaxNy,HEAD_SPM1+51,2);
    memcpy(&what_this,HEAD_SPM1+59,2);
    while(what_this)//Subfield what_this contains info about stored fields witch we can use to find amount of those.
    {
        field_amount++;
        what_this &= (what_this-1); //current AND itself-1 to remove the least bit. repeat untill its 0.
    }
    char notifications[32][336];//Getting all notifications about stored fields of data.
    for(int i=0;i<field_amount;i++)
        data.readRawData(notifications[i], 336);

    char fieldName[32][32], scaleXYname[32][6], scaleZname[32][6];//info from notifications about fields
    float scaleX[32], scaleY[32], scaleZ[32];
    for(int i=0; i<field_amount;i++)//Getting that information.
    {
        strncpy(fieldName[i], notifications[i], 32);
        strncpy(scaleXYname[i], notifications[i]+68, 6);
        strncpy(scaleZname[i], notifications[i]+74, 6);
        memcpy(&Nx[i], notifications[i]+34, 2);
        memcpy(&Ny[i], notifications[i]+36, 2);
        memcpy(&scaleX[i], notifications[i]+40, 4);
        memcpy(&scaleY[i], notifications[i]+44, 4);
        memcpy(&scaleZ[i], notifications[i]+48, 4);
    }


    //Getting all raw data
    for(int i=0; i<field_amount;i++)
        data.readRawData(dataRaw[i], Nx[i]*Ny[i]*2);
    for(int i=0; i<field_amount;i++)//Trasform raw data into 16bit integers.
    {
        for(int j=0; j<(Nx[i]*Ny[i]);j++)
        {
            memcpy(&dataShort[i][j], dataRaw[i]+(2*j), 2);
        }
    }
    for(int i=0; i<field_amount;i++)//Multiping data by z-multiplier
    {
        for(int j=0; j<(Nx[i]*Ny[i]);j++)
        {
            dataMuliplied[i][j]=dataShort[i][j]*scaleZ[i];
        }
    }


    bool fwCheck, bwCheck, isFwExist=false, isBwExist=false;
    short fwFieldNumber, bwFieldNumber;
    QRegExp fwRx("Torsion"), bwRx("Torsion....");//Simple regular expression and an exact match.
    for(int i=0;i<field_amount;i++)
    {
        fwCheck=fwRx.exactMatch(fieldName[i]);
        bwCheck=bwRx.exactMatch(fieldName[i]);
        if(fwCheck) {fwFieldNumber=i; isFwExist=true;}
        if(bwCheck) {bwFieldNumber=i; isBwExist=true;}
    }//Getting numbers of input fields for friction calculaton.

    if(!isFwExist || !isBwExist)
    {
        return InputFileName + ": Ошибка: минимум 1 поле, необходимое для расчетов, не найдено.\n";
    }

    //Calculate delta of fields.
    float deltaFieldF = find_delta_field(dataMuliplied[fwFieldNumber], (Nx[fwFieldNumber]*Ny[fwFieldNumber]), Ny[fwFieldNumber]),
          deltaFieldB = find_delta_field(dataMuliplied[bwFieldNumber], (Nx[bwFieldNumber]*Ny[bwFieldNumber]), Ny[bwFieldNumber]);
    float DZ = abs(deltaFieldF - deltaFieldB)/2;//Main variable for friction coeff. calculation.


    //int fieldSize = Nx[fwFieldNumber]*Ny[fwFieldNumber];
    double ktr=(2*DZ*l)/(3*(1+v)*(65535*procent)*s*2); // friction coeff.
    double Zreal=DZ/koeffkolibrovki;
    double Zn=(65535)/koeffkolibrovki;
    double Ffr=Zreal*l*k/(3*(1+v)*s);
    //double Ftr=((DZ/2)*l*k)/(3*s*(1+v));
    double Fn=k*Zn;





    /*return InputFileName + ": Расчет завершен успешно.\n" + "DZ: " + QString::number(DZ) +  "; ktr: " + QString::number(ktr)
           + "; Zreal: " + QString::number(Zreal) + "; Zn: " + QString::number(Zn) + "; Ftr: " + QString::number(Ffr) +  "\n";*/

    return "\n" + InputFileName + ": Расчет завершен успешно" + "\t" + QString::number(DZ) + "\t" + QString::number(ktr)
           /*+ "\t±\t" + QString::number(dKtr) */+ "\t" + QString::number(Ffr) /*+ "\t±\t" + QString::number(dFfr)*/
            + "\t" + QString::number(Fn);
}

void S2F::dirSpm2friction()
{
    if(InputFileDir == "")//Default directory
    {
        QMessageBox MTname;
        MTname.setText("Не выбрана папка. Пожалуйста, выберите.");
        MTname.exec();
        InputFileDir = QFileDialog::getExistingDirectory(this, tr("Выбрать папку"), "/", QFileDialog::ShowDirsOnly);
        ui->lineEditIn_5->setText(InputFileDir);
        return;
    }
    QDirIterator fileList(InputFileDir, QDirIterator::NoIteratorFlags);//Get all files in chosen directory.
    QStringList files;
    while(fileList.hasNext())
    {
        files.append(fileList.next());
    }
    QStringList spmFiles;//Now we get all .spm files from given.
    QRegExp spm("*.spm");
    spm.setPatternSyntax(QRegExp::Wildcard);
    for(qint32 i=0;i<files.size();i++)
    {
        if(spm.exactMatch(files[i]))
        {
            spmFiles.append(files[i]);
        }
    }

    OutputFileName = InputFileDir + "/friction_all_txt.txt";
    QFile out(OutputFileName);
    if(!out.open(QIODevice::WriteOnly))
        return;
    QTextStream outText(&out);
    QList<QString> resultList;
    QString MT = NULL;
    outText << MT + "Имя файла\tDZ/2\tKfr\tFfr\tFn\t";
    for(qint32 i=0;i<spmFiles.size();i++)//And now we just call our function to calculate friction.
    {
        InputFileName=spmFiles[i];
        resultList << spm2friction();
                       //That function works with files one by one, so we change the input filename
    }                  //to process each individual .spm file we found earlier.
                       //And yes, the function checks for spm1 format and presence of both fields for calculation.
    QList<float> DZ,Kfr,Ffr,Fn;
    float DZavr=0, Kfravr=0, Ffravr=0, Fnavr=0;
    QRegExp rx("\\d+\\.\\d+");
    for(int i=0;i<resultList.size();i++)
    {
        int posF = 0;
        QStringList listF;
        while ((posF = rx.indexIn(resultList[i], posF)) != -1)
        {
                posF+= rx.matchedLength();
                listF.append(rx.capturedTexts());
        }
        DZ.append(listF[0].toFloat());
        Kfr.append(listF[1].toFloat());
        Ffr.append(listF[2].toFloat());
        Fn.append(listF[3].toFloat());
        DZavr+=DZ[i];
        Kfravr+=Kfr[i];
        Ffravr+=Ffr[i];
        Fnavr+=Fn[i];
    }
    DZavr/=DZ.size();
    Kfravr/=Kfr.size();
    Ffravr/=Ffr.size();
    Fnavr/=Ffr.size();
    //now, standart deviation and confidence interval
    float ZSqrDif=0;
    for(int i=0;i<DZ.size();i++)
    {
        ZSqrDif+=pow((DZ[i]-DZavr),2);
    }
    float ZStdDev = sqrt(ZSqrDif/(DZ.size()-1));
    float ZConfInt = (1.96*ZStdDev)/(sqrt(DZ.size()));

    //now confidence intervals for calculated.
    float dKtr = Kfravr/DZavr*ZConfInt;
    float dFfr = (ZConfInt*l*k)/(koeffkolibrovki*3*(1+v)*s);
    /*outText << MT + "Средние:\n" + "DZ\t" + QString::number(DZavr) + "\t±\t" + QString::number(ZConfInt)
               + "\tKfr\t" + QString::number(Kfravr) + "\t±\t" + QString::number(dKtr) + "\tFfr\t"
               + QString::number(Ffravr) + "\t±\t" + QString::number(dFfr) + "\tFn\t" + QString::number(Fnavr);
    outText << MT +  "\nКонстанты:\n" +  "Длина кантеливера, м\t" + QString::number(l) +  "\tКоэффициент жесткости, Н/м\t" + QString::number(k)
               +  "\tВысота иглы, м\t" + QString::number(s) +  "\tКоэффициент Пуассона\t" + QString::number(v) +  "\tНагрузка, %/100\t" + QString::number(procent)
               +  "\tКалибровачный коэффициент\t" + QString::number(koeffkolibrovki) + "\n\n";
    */

    if(resultList.size()<8)
    {
        for(int i=resultList.size();i<8;i++)
        {
            resultList.append("\n\t\t\t\t");
        }
    }

    resultList[0].append("\t\t\t\tСредние");
    resultList[1].append("\t\t\t\tDZ/2\t\t\tKft\t\t\tFfr\t\t\tFn");
    resultList[2].append("\t\t\t\t" + QString::number(DZavr) + "\t±\t" + QString::number(ZConfInt)
                         + "\t" + QString::number(Kfravr) + "\t±\t" + QString::number(dKtr) + "\t"
                         + QString::number(Ffravr) + "\t±\t" + QString::number(dFfr) + "\t"
                         + QString::number(Fnavr));
    resultList[5].append("\t\t\t\tКонстанты");
    resultList[6].append("\t\t\t\tДлина кантеливера, м\tКоэффициент жесткости, Н/м\tВысота иглы, м\tКоэффициент Пуассона\tНагрузка, %/100\tКалибровачный коэффициент\t");
    resultList[7].append("\t\t\t\t" + QString::number(l) + "\t" + QString::number(k)
                         + "\t" + QString::number(s) + "\t" + QString::number(v) + "\t"
                         + QString::number(procent) + "\t" + QString::number(koeffkolibrovki));
    for(int i = 0;i<resultList.size();i++)
    {
        outText << resultList[i];
        ui->progressBar->setValue((i+1)*100/spmFiles.size());
    }
    outText.flush();
    out.close();
    QMessageBox confirmation;
    confirmation.setText("Расчет завершен. Файл с результатами могут быть найдены в папке с исходнымы файлами.");
    confirmation.exec();
    InputFileName="";
}

void S2F::on_BrowseInButton_clicked()
{
    InputFileName = QFileDialog::getOpenFileName(this, tr("Открыть .spm файл"),InputFileName, tr("Spm files (*.spm)"));
    ui->lineEditIn->setText(InputFileName);
}

void S2F::on_BrowseBwButton_clicked()
{
    InputFileNameBw = QFileDialog::getOpenFileName(this, tr("Открыть .txt файл обратного хода"),InputFileNameBw, tr("txt files (*torsion*bw*.txt)"));
    ui->lineEditBw->setText(InputFileNameBw);
}

void S2F::on_Spm2TxtButton_clicked()
{
    ReadSpm();
}

void S2F::on_BrowseFwButton_clicked()
{
    InputFileNameFw = QFileDialog::getOpenFileName(this, tr("Открыть .txt файл прямого хода"),InputFileNameFw, tr("txt files (*torsion_*.txt)"));
    ui->lineEditFw->setText(InputFileNameFw);
}

void S2F::on_calcFButton_clicked()
{
    CalculateFriction();
}

void S2F::on_About1Button_clicked()
{
    QMessageBox about;
    about.setText("Распаковка файла формата spm1 в несколько txt файлов, содержающие поля с информацией. Файлы будут созданы в папке с исходным файлом.");
    about.exec();
}

void S2F::on_About2Button_clicked()
{
    QMessageBox about;
    about.setText("Расчет трения на основе txt файлов. Выбираются файлы для прямого и обратного ходов. Файл будет создан в папке с файлом прямого хода");
    about.exec();
}

void S2F::on_BrowseInButton_2_clicked()
{
    InputFileName = QFileDialog::getOpenFileName(this, tr("Открыть .spm файл"),InputFileName, tr("Spm files (*.spm)"));
    ui->lineEditIn_2->setText(InputFileName);
}

void S2F::on_About3Button_clicked()
{
    QMessageBox about;
    about.setText("Расчет трения на основе spm1 файла. Файл будет создан в папке с файлом прямого хода");
    about.exec();
}

void S2F::on_Spm2FrictionButton_clicked()
{
    QString result = spm2friction();
    if(result != "")
    {
        int extPointPos = InputFileName.lastIndexOf(".");//Generating filenames based on input filename
        OutputFileName = InputFileName.left(extPointPos) + "_friction_txt.txt";
        QFile out(OutputFileName);
        if(!out.open(QIODevice::WriteOnly))
            return;
        QTextStream outText(&out);
        QString MT = NULL;
        outText << MT + "Имя файла\tDZ/2\tKfr\tFfr\tFn\t";
        outText << result +  "\nКонстанты:\n" +  "Длина кантеливера, м\t" + QString::number(l) +  "\tКоэффициент жесткости, Н/м\t" + QString::number(k)
                   +  "\tВысота иглы, м\t" + QString::number(s) +  "\tКоэффициент Пуассона\t" + QString::number(v) +  "\tНагрузка, %/100\t" + QString::number(procent)
                   +  "\tКалибровачный коэффициент\t" + QString::number(koeffkolibrovki) + "\n\n";
        outText.flush();
        out.close();
        QMessageBox confirmation;
        confirmation.setText("Расчет завершен. Файл с результатами могут быть найдены в папке с исходным файлом.");
        confirmation.exec();
    }

}

void S2F::on_About4Button_clicked()
{
    QMessageBox about;
    about.setText("Расчет трения на основе всех spm1 файлой в выбранной директории. Файлы будут созданы в папке с иходными файлами");
    about.exec();
}

void S2F::on_BrowseInButton_5_clicked()
{
    InputFileDir = QFileDialog::getExistingDirectory(this, tr("Выбрать папку"), InputFileDir, QFileDialog::ShowDirsOnly);
    ui->lineEditIn_5->setText(InputFileDir);
}

void S2F::on_Spm2FrictionButton_Dir_clicked()
{
    dirSpm2friction();
}

void S2F::on_procentEdit_textChanged()
{
    procent = ui->procentEdit->text().replace(",",".").toDouble();
}

void S2F::on_koeffkalEdit_textChanged()
{
    koeffkolibrovki = ui->koeffkalEdit->text().replace(",",".").toDouble();
}

void S2F::on_lEdit_textChanged()
{
    l = ui->lEdit->text().replace(",",".").toDouble();
}

void S2F::on_koeffEdit_textChanged()
{
    k = ui->koeffEdit->text().replace(",",".").toDouble();
}

void S2F::on_sEdit_textChanged()
{
    s = ui->sEdit->text().replace(",",".").toDouble();
}

void S2F::on_vEdit_textChanged()
{
    v = ui->vEdit->text().replace(",",".").toDouble();
}

void S2F::on_lineEditIn_2_textChanged()
{
    InputFileName = ui->lineEditIn_2->text();
}

void S2F::on_lineEditIn_5_textChanged()
{
    InputFileDir = ui->lineEditIn_5->text();
}

void S2F::on_lineEditFw_textChanged()
{
    InputFileNameFw = ui->lineEditFw->text();
}

void S2F::on_lineEditBw_textChanged()
{
    InputFileNameBw = ui->lineEditBw->text();
}

void S2F::on_lineEditIn_textChanged()
{
    InputFileName = ui->lineEditIn->text();
}
