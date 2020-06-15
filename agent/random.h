#ifndef _MYRNG_H
#define _MYRNG_H

namespace Random {

int randint(int l, int r);
bool decide(float p);            // 0 & 1
int choice(int n, const float p[]);

}

#endif
