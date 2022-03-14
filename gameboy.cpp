#include <bits/stdc++.h>
#include "Z80.h"
#include "mainwindow.h"
#include <QDebug>
#include <QApplication>

using namespace std;

char* rom;

//Video Memory
unsigned char graphicsRAM[8192];
int palette[4];
int tileset, tilemap, scrollx, scrolly;

unsigned char memoryRead(int address) {
    return rom[address];
}

void memorywrite(int address, unsigned char value) {

}

void renderScreen(){
     for (int row = 0; row < 144 ; row++)
    {
        for (int column = 0; column < 160; column++)
        {
            int x=row,y=column;
            int tilex = x/8, tiley = y/8;
            int tileposition = tiley * 32 + tilex;
            int tileindex, tileaddress;
            if (tilemap){ //tilemap1
                tileindex = graphicsRAM[0x1c00+tileposition];
                tileaddress = tileindex * 16;
            }
            else { //tilemap0
                tileindex = graphicsRAM[0x1800+tileposition];
                if (tileindex >= 128)
                    tileaddress = tileindex-256;
                else
                    tileaddress = tileindex * 16 + 0x1000;
            }

            int xoffset =  x%8, yoffset = y%8; //should use &
            int row0 = graphicsRAM[tileaddress + yoffset*2];
            int row1 = graphicsRAM[tileaddress + yoffset*2 + 1];

            int row0shifted = row0>>(7-xoffset), row0capturepixel = row0shifted & 1;
            int row1shifted = row1>>(7-xoffset), row1capturepixel = row1shifted & 1;

            int pixel = row1capturepixel * 2 + row0capturepixel;
            int color = palette[pixel];

            updateSquare(x,y,color);
        }
    }
    onFrame();
}

/*! Documentation
 * \brief main The starting point of the application
 * \param argc The argument count
 * \param argv The arguments
 * \return  The return value of the application
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qInfo() << "Part 1";
    qInfo() << "Reading Rom from a File";
    ifstream romfile("testrom.gb", ios::in|ios::binary|ios::ate); // Beware! copy the rom file to the output directory
    streampos size = romfile.tellg();
    qInfo() << "Rom Size: " << size;
    rom = new char[size];
    int romSize=size;
    romfile.seekg(0,ios::beg);
    romfile.read(rom,size);
    romfile.close();
    qInfo() << "Displaying the Window";
    setup(argc, argv);
    qInfo() << "Part 2";
    qInfo() << "Reading the graphics dump into memory";
    int n;
        ifstream vidfile("screendump.txt",ios::in);
        for(int i=0; i<8192; i++){
            int n;
            vidfile>>n;
            graphicsRAM[i]=(unsigned char)n;
        }
    vidfile >> tileset;
    vidfile >> tilemap;
    vidfile >> scrollx;
    vidfile >> scrolly;
    vidfile >> palette[0];
    vidfile >> palette[1];
    vidfile >> palette[2];
    vidfile >> palette[3];
    renderScreen();

//    qInfo() << "Creating the Z80 Object";
//    Z80* z80 = new Z80(memoryRead, memorywrite);
//    z80 -> reset();

//    while(!z80->halted){
//        z80->doInstruction();
//        qInfo() << "PC: " << z80-> PC << "A: "<< z80->A<< " B: "<< z80->B ;
//    }

    return a.exec();
}
