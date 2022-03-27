#include <bits/stdc++.h>
#include "Z80.h"
#include "mainwindow.h"
#include <QDebug>
#include <QApplication>

using namespace std;

char *rom;

// Z80 CPU
Z80 *z80;

// Video Memory
unsigned char graphicsRAM[8192];
int palette[4];
int tileset, tilemap, scrollx, scrolly;

// Memory Map
int HBLANK = 0, VBLANK = 1, SPRITE = 2, VRAM = 3;
unsigned char workingRAM[0x2000];

unsigned char page0RAM[0x80];

int line = 0, cmpline = 0, videostate = 0, keyboardColumn = 0, horizontal = 0;
int gpuMode = HBLANK;
int romOffset = 0x4000;
long totalInstructions = 0;

// Keyboard Keys
int keys1 = 0xf;
int keys0 = 0xf;

// Memory Bank
int rombank, cartridgetype, romsizemask;

// Sprites
unsigned char spriteRAM[0x100];
int objpalette0[4];
int objpalette1[4];

void dma(int);

unsigned char getKey()
{
    return 0xf;
}

void setControlByte(unsigned char b)
{
    tilemap = (b & 8) != 0 ? 1 : 0;
    tileset = (b & 16) != 0 ? 1 : 0;
}

void setPalette(unsigned char b)
{
    palette[0] = b & 3;
    palette[1] = (b >> 2) & 3;
    palette[2] = (b >> 4) & 3;
    palette[3] = (b >> 6) & 3;
}

void setObjpalette0(unsigned char b)
{
    objpalette0[0] = b & 3;
    objpalette0[1] = (b >> 2) & 3;
    objpalette0[2] = (b >> 4) & 3;
    objpalette0[3] = (b >> 6) & 3;
}
void setObjpalette1(unsigned char b)
{
    objpalette1[0] = b & 3;
    objpalette1[1] = (b >> 2) & 3;
    objpalette1[2] = (b >> 4) & 3;
    objpalette1[3] = (b >> 6) & 3;
}

unsigned char getVideoState()
{
    int by = 0;

    if (line == cmpline)
        by |= 4;

    if (gpuMode == VBLANK)
        by |= 1;

    if (gpuMode == SPRITE)
        by |= 2;

    if (gpuMode == VRAM)
        by |= 3;

    return (unsigned char)((by | (videostate & 0xf8)) & 0xff);
}

void keydown(int value)
{
    qInfo() << "Key pressed: " << value;
    if (value == 32 || value == 114) // right, D
        keys1 &= 0xe;
    else if (value == 30 || value == 113) // left, A
        keys1 &= 0xd;
    else if (value == 17 || value == 111) // Up, W or arrow
        keys1 &= 0xb;
    else if (value == 31 || value == 116) // Down, S or arrow
        keys1 &= 0x7;
    else if (value == 57 || value == 45) // A, Z or space
        keys0 &= 0xe;
    else if (value == 36 || value == 46) // B, X or J
        keys0 &= 0xd;
    else if (value == 1 || value == 119) // Start, ESC or DEL
        keys0 &= 0xb;
    else if (value == 42 || value == 62) // Select, LSHIFT or RSHIFT
        keys0 &= 0x7;

    z80->throwInterrupt(0x10);
}

void keyup(int value)
{
    qInfo() << "Key depressed: " << value;
    if (value == 32 || value == 114) // right, D or arrow
        keys1 |= 1;
    else if (value == 30 || value == 113) // left, A or arrow
        keys1 |= 2;
    else if (value == 17 || value == 111) // Up, W or arrow
        keys1 |= 4;
    else if (value == 31 || value == 116) // Down, S or arrow
        keys1 |= 8;
    else if (value == 57 || value == 45) // A, Z or space
        keys0 |= 1;
    else if (value == 36 || value == 46) // B, X or J
        keys0 |= 2;
    else if (value == 1 || value == 119) // Start, esc or del
        keys0 |= 4;
    else if (value == 42 || value == 62) // Select, Lshift or Rshift
        keys0 |= 8;

    z80->throwInterrupt(0x10);
}

unsigned char memoryRead(int address)
{
    if (address >= 0 && address <= 0x3FFF)
        return rom[address];
    else if (address >= 0x4000 && address <= 0x7FFF)
        return rom[romOffset + address % 0x4000];
    else if (address >= 0x8000 && address <= 0x9FFF)
        return graphicsRAM[address % 0x2000];
    else if (address >= 0xC000 && address <= 0xDFFF)
        return workingRAM[address % 0x2000];
    else if (address >= 0xfe00 && address <= 0xfe9f)
        return spriteRAM[address % 0xff];
    else if (address >= 0xFF80 && address <= 0xFFFF)
        return page0RAM[address % 0x80];
    else if (address == 0xff00)
    {
        if ((keyboardColumn & 0x30) == 0x10)
            return keys0;
        else
            return keys1;
    }

    else if (address == 0xFF41)
        return getVideoState();
    else if (address == 0xFF42)
        return scrolly;
    else if (address == 0xFF43)
        return scrollx;
    else if (address == 0xFF44)
        return line;
    else if (address == 0xFF45)
        return cmpline;
    else
        return 0;
}

void memoryWrite(int address, unsigned char value)
{
    if (cartridgetype == 1 || cartridgetype == 2 || cartridgetype == 3)
    {
        if (address >= 0x2000 && address <= 0x3fff)
        {
            value = value & 0x1f;
            if (value == 0)
                value = 1;

            rombank = rombank & 0x60;
            rombank += value;
            romOffset = (rombank * 0x4000) & romsizemask;
        }
        else if (address >= 0x4000 && address <= 0x5fff)
        {
            value = value & 3;

            rombank = rombank & 0x1f;
            rombank |= value << 5;

            romOffset = (rombank * 0x4000) & romsizemask;
        }
    }
    if (address >= 0x8000 && address <= 0x9FFF)
        graphicsRAM[address % 0x2000] = value;
    else if (address >= 0xC000 && address <= 0xDFFF)
        workingRAM[address % 0x2000] = value;
    else if (address >= 0xFE00 && address <= 0xFE9F)
    {
        spriteRAM[address & 0xff] = value;
    }
    else if (address >= 0xFF80 && address <= 0xFFFF)
        page0RAM[address % 0x80] = value;
    else if (address == 0xFF00)
        keyboardColumn = value;
    else if (address == 0xFF40)
        setControlByte(value);
    else if (address == 0xFF41)
        videostate = value;
    else if (address == 0xFF42)
        scrolly = value;
    else if (address == 0xFF43)
        scrollx = value;
    else if (address == 0xFF44)
        line = value;
    else if (address == 0xFF45)
        cmpline = value;
    else if (address == 0xFF46)
        dma(value);
    else if (address == 0xFF47)
        setPalette(value);
    else if (address == 0xFF48)
        setObjpalette0(value);
    else if (address == 0xff49)
        setObjpalette1(value);
    else if (address >= 0xFF80 && address <= 0xFFFF)
        page0RAM[address % 0x80] = value;
    else
        qInfo() << value << " is not Implemented.";
}

void dma(int address)
{
    address = address << 8;
    for (int i = 0; i < 0xa0; i++)
        memoryWrite(0xfe00 + i, memoryRead(address + i));
}

void renderScreen(int row)
{
    for (int column = 0; column < 160; column++)
    {
        // apply scroll
        // SWAPPED X and Y
        int y = row, x = column;

        x = (x + scrollx) & 255;
        y = (y + scrolly) & 255;

        // determine which tile pixel belongs to
        int tilex = x / 8, tiley = y / 8;
        // int tilex = ( (x + scrollx)&255 )/8, tiley = ( (y + scrolly)&255 )/8;
        // find tileposition in 1D array
        int tileposition = tiley * 32 + tilex;

        int tileindex = 0, tileaddress = 0;

        if (tilemap) // tilemap1
            tileindex = graphicsRAM[0x1c00 + tileposition];
        else // tilemap0
            tileindex = graphicsRAM[0x1800 + tileposition];

        if (tileset)
            tileaddress = tileindex * 16;
        else
        {
            if (tileindex >= 128)
                tileindex -= 256;
            tileaddress = tileindex * 16 + 0x1000;
        }

        // get which pixel is in the tile
        int xoffset = x % 8, yoffset = y % 8; // should use &
        // get the two bytes for each row in tile
        int row0 = graphicsRAM[tileaddress + yoffset * 2];
        int row1 = graphicsRAM[tileaddress + yoffset * 2 + 1];

        // Binary math to get binary indexed color info across both bytes
        int row0shifted = row0 >> (7 - xoffset);
        int row0capturepixel = row0shifted & 1;

        int row1shifted = row1 >> (7 - xoffset);
        int row1capturepixel = row1shifted & 1;

        // combine byte info to get color
        int pixel = row1capturepixel * 2 + row0capturepixel;
        // get color based on palette
        int color = palette[pixel];

        // Sprites
        for (int spnum = 0; spnum < 40; spnum++)
        {
            int spritey = spriteRAM[spnum * 4 + 0] - 16;
            int spritex = spriteRAM[spnum * 4 + 1] - 8;
            int tilenumber = spriteRAM[spnum * 4 + 2];
            int options = spriteRAM[spnum * 4 + 3];

            // indexes found on imrannazer.com link

            if (spritex + 8 <= column || spritex > column)
                continue;
            else if (spritey + 8 <= row || spritey > row)
                continue;
            else if ((options & 0x80) != 0)
                continue;
            else
            {

                tileaddress = tilenumber * 16;
                xoffset = column - spritex;
                yoffset = row - spritey;
                if ((options & 0x40) != 0)
                    yoffset = 7 - yoffset;
                if ((options & 0x20) != 0)
                    xoffset = 7 - xoffset;

                row0 = graphicsRAM[tileaddress + yoffset * 2];
                row1 = graphicsRAM[tileaddress + yoffset * 2 + 1];

                row0shifted = row0 >> (7 - xoffset);
                row1shifted = row1 >> (7 - xoffset);

                row0capturepixel = row0shifted & 1;
                row1capturepixel = row1shifted & 1;

                int palindex = row1capturepixel * 2 + row0capturepixel;

                if (palindex == 0)
                {
                }
                else
                {
                    if ((options & 0x10) == 0)
                        color = objpalette0[palindex];
                    else
                        color = objpalette1[palindex];
                }
            }
        }

        updateSquare(column, row, color);
    }
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
    qInfo() << "Gameboy Emulator by Jayapraveen";
    qInfo() << "Part 1";
    qInfo() << "Reading Rom from a File";
    ifstream romfile("mario.gb", ios::in | ios::binary | ios::ate); // Beware! copy the rom file to the output directory
    streampos size = romfile.tellg();
    qInfo() << "Rom Size: " << size;
    rom = new char[size];
    int romSize = size;
    romfile.seekg(0, ios::beg);
    romfile.read(rom, size);
    romfile.close();
    qInfo() << "Displaying the Window";
    setup(argc, argv);
    qInfo() << "Part 5";
    rombank = 0;
    cartridgetype = rom[0x147] & 3;
    int buff[] = {0x7fff, 0xffff, 0x1ffff, 0x3ffff, 0x7ffff, 0xfffff, 0x1fffff, 0x3fffff, 0x7fffff};
    int adr = rom[0x148];
    romsizemask = buff[adr];
    qInfo() << "Creating the z80 object";
    z80 = new Z80(memoryRead, memoryWrite);
    z80->reset();
    while (true)
    {
        if (!z80->halted) // if not halted, do an instruction
            z80->doInstruction();

        if (z80->interrupt_deferred > 0) // check for and handle interrupts
        {
            z80->interrupt_deferred--;
            if (z80->interrupt_deferred == 1)
            {
                z80->interrupt_deferred = 0;
                z80->FLAG_I = 1;
            }
        }
        z80->checkForInterrupts();

        // figure out the screen position and set the video mode

        int horizontal = (int)((totalInstructions + 1) % 61); //(int) ((instructions+1)%61)
        if (line >= 145)
            gpuMode = VBLANK;
        else if (horizontal <= 30)
            gpuMode = HBLANK;
        else if (horizontal >= 31 && horizontal <= 40)
            gpuMode = SPRITE;
        else
            gpuMode = VRAM;

        if (horizontal == 0)
        {
            line++;
            if (line == 144)
                z80->throwInterrupt(1);
            if (line >= 0 && line < 144)
            {
                renderScreen(line);
            }
            if (line % 153 == cmpline && (videostate & 0x40) != 0)
                z80->throwInterrupt(2);
            if (line == 153)
            {
                line = 0;
                onFrame(); // redraw the screen
            }
        }
        totalInstructions++;
    }
    // return a.exec();
}
