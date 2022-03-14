#include <bits/stdc++.h>
#include "Z80.h"
#include <QDebug>
#include <QCoreApplication>

using namespace std;

//Rom array containing a sample program
//unsigned char rom[]={0x06,0x06,0x3e,0x00,0x80,0x05,0xc2,0x04,0x00,0x76};

char* rom;

unsigned char memoryRead(int address) {
    return rom[address];
}

void memorywrite(int address, unsigned char value) {

}

/*! Documentation
 * \brief main The starting point of the application
 * \param argc The argument count
 * \param argv The arguments
 * \return  The return value of the application
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
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
    qInfo() << "Creating the Z80 Object";
    Z80* z80 = new Z80(memoryRead, memorywrite);
    z80 -> reset();
    //z80 -> PC = 0;

//    z80 -> doInstruction();
//    qInfo() << "PC: " << z80-> PC << "A: "<< z80->A<< " B: "<< z80->B ;


    while(!z80->halted){
        z80->doInstruction();
        qInfo() << "PC: " << z80-> PC << "A: "<< z80->A<< " B: "<< z80->B ;
    }

    return a.exec();
}
