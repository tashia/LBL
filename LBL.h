#ifndef LBL_H
#define LBL_H

// EngExtCpp class name
// This define must be provided BEFORE including engextcpp.hpp.  This is because the header
// automatically references a class named EXT_CLASS to be used for all extension activity.
//
#define EXT_CLASS LBL_EXT

#include <engextcpp.hpp>


class LBL_EXT : public ExtExtension
{
public:
	EXT_COMMAND_METHOD(memsave);

protected:
	bool IsLiveTarget();
};

#endif  // LBL_H