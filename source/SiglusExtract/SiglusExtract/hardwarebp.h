#ifndef _HARDWAREBP_H_
#define _HARDWAREBP_H_


class HardwareBreakpoint
{
public:
	HardwareBreakpoint() { m_index = -1; }
	~HardwareBreakpoint() { Clear(); }

	// The enum values correspond to the values used by the Intel Pentium,
	// so don't change them!
	enum Condition { Write = 1, Read /* or write! */ = 3 };

	void Set(void* address, int len /* 1, 2, or 4 */, Condition when);
	void Clear();
	void Clear(PCONTEXT Context);

protected:

	inline void SetBits(unsigned long& dw, int lowBit, int bits, int newValue)
	{
		int mask = (1 << bits) - 1; // e.g. 1 becomes 0001, 2 becomes 0011, 3 becomes 0111

		dw = (dw & ~(mask << lowBit)) | (newValue << lowBit);
	}

	int m_index; // -1 means not set; 0-3 means we've set that hardware bp
};


#endif // _HARDWAREBP_H_