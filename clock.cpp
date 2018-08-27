#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <unistd.h>

using namespace std;

//
//      A
//     ___
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D
//
// Above: LCD number display diagram
//
// array positions are ordered to represent A, B, C, D, E, F, G from above diagram

const static int gpio_map[3][7] = {{61, 14, 49, 15, 3, 65, 46}, {44, 26, 2, 5, 4, 68, 67}, {69, 66, 51, 50, 60, 45, 23}};
const static bool num_map[11][7] = { {1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0}, {1, 1, 0, 1, 1, 0, 1},
                                   {1, 1, 1, 1, 0, 0, 1}, {0, 1, 1, 0, 0, 1, 1}, {1, 0, 1, 1, 0, 1, 1},
                                   {1, 0, 1, 1, 1, 1, 1}, {1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 1, 1},
                                   {1, 1, 1, 1, 0, 1, 1}, {0, 0, 0, 0, 0, 0, 0} };

class Nums
{
public:
    void display(int n, int place); // m is digit slot, n is display number   
    void wipe(); 
private:
    void gpio_write(string sdir, bool snum);
};

// set the LCD to have a blank display
void Nums::wipe()
{
    string sdir;
    ostringstream oss;

    for(int i=0; i<3; ++i){
        for(int j=0; j<7; ++j){
            sdir = "/sys/class/gpio/gpio"; 
            oss.str("");
            oss << gpio_map[i][j];
            sdir += oss.str();
            sdir += "/value";   
            gpio_write(sdir, false);     
        }
    }
    sdir = "/sys/class/gpio/gpio47/value"; // gpio for B on left most digit
    gpio_write(sdir, false);
    sdir = "/sys/class/gpio/gpio48/value"; // gpio for C on left most digit
    gpio_write(sdir, false);
}

void Nums::gpio_write(string sdir, bool snum)
{
    ofstream fout(sdir.c_str());
    
    fout << snum;
    fout.close();
}

void Nums::display(int n, int place)
{
    string sdir;
    ostringstream oss;
    
    if(place == 3){ // left most digit display numeral 1 or nothing, driven by 2 ports only
        sdir = "/sys/class/gpio/gpio47/value"; // gpio for B on left most digit
        gpio_write(sdir, n);
        sdir = "/sys/class/gpio/gpio48/value"; // gpio for C on left most digit
        gpio_write(sdir, n);
        return;
    }
    for(int i=0; i<7; ++i){
        sdir = "/sys/class/gpio/gpio"; 
        oss.str("");
        oss << gpio_map[place][i];
        sdir += oss.str();
        sdir += "/value";
        gpio_write(sdir, num_map[n][i]);
    }
}

int main()
{
    Nums nums;
    time_t rawtime;
    int secs_left;
    int min, hour;
    struct tm *timeinfo;
    
    while(true){
        nums.wipe(); // give LCD a brief rest (seems to help legibility of screen), rough pwm
        usleep(12000); // sleep for 4ms
        rawtime = time(NULL);
        timeinfo = localtime(&rawtime);
        timeinfo = localtime(&rawtime);
        min = timeinfo -> tm_min;
        hour = timeinfo -> tm_hour;
        nums.display(min%10, 0);
        nums.display(min/10, 1);
        if(hour > 12) hour -= 12;
        nums.display(hour%10, 2);
        nums.display(hour/10, 3);
        usleep(20000); // sleep for 40ms
    } 
}

