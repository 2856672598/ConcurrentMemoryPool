#pragma once

#include "common.h"

//ÖÐÐÄ»º´æ
class CentreCache
{
public:
	std::mutex _mlock;
private:
	SpanList _spanList[NSPANLIST];
};