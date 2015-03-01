#ifndef MISC_ARGUMENTPARSERS_H
#define MISC_ARGUMENTPARSERS_H

namespace Misc
{

void parseInt  ( const char* s, int&   i, int   clampLo=0, int   clampHi=-1 );
void parseFloat( const char* s, float& f, float clampLo=0, float clampHi=-1 );

} // namespace Misc

#endif