#ifndef S2F_H
#define S2F_H

#include <QMainWindow>

namespace Ui {
class S2F;
}

class S2F : public QMainWindow
{
    Q_OBJECT

public:
    QString InputFileName;
    QString OutputFileName;
    QString InputFileNameFw;
    QString InputFileNameBw;
    QString InputFileDir;
    double koeffkolibrovki=219.0;

    double l=0.0002, k=2.70, s=0.000015, v=0.3, procent=0.2;
    void ReadSpm();
    //void ReadSPM0(QDataStream&);
    void ReadSPM1(QDataStream&);
    //void ReadMPS(QDataStream&);
    void CalculateFriction();
    QString spm2friction();
    void dirSpm2friction();
    explicit S2F(QWidget *parent = 0);
    ~S2F();

private slots:
    void on_BrowseInButton_clicked();

    void on_BrowseBwButton_clicked();

    void on_Spm2TxtButton_clicked();

    void on_BrowseFwButton_clicked();

    void on_calcFButton_clicked();

    void on_About1Button_clicked();

    void on_About2Button_clicked();

    void on_BrowseInButton_2_clicked();

    void on_About3Button_clicked();

    void on_Spm2FrictionButton_clicked();

    void on_About4Button_clicked();

    void on_BrowseInButton_5_clicked();

    void on_Spm2FrictionButton_Dir_clicked();

    void on_procentEdit_textChanged();

    void on_koeffkalEdit_textChanged();

    void on_lEdit_textChanged();

    void on_koeffEdit_textChanged();

    void on_sEdit_textChanged();

    void on_vEdit_textChanged();

    void on_lineEditIn_2_textChanged();

    void on_lineEditIn_5_textChanged();

    void on_lineEditFw_textChanged();

    void on_lineEditBw_textChanged();

    void on_lineEditIn_textChanged();

private:
    Ui::S2F *ui;
};

#endif // S2F_H
