#pragma once

#include "common.h"

//���Ļ���
class CentreCache
{
public:
	std::mutex _mlock;
private:
	SpanList _spanList[NSPANLIST];
};