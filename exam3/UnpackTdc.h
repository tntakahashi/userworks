/*
 *
 *
 */

#include <iostream>
#include <iomanip>


namespace TDC40 {

static constexpr unsigned int T_TDC = 0xd;
static constexpr unsigned int T_HB = 0xf;
static constexpr unsigned int T_ERROR = 0xe;
static constexpr unsigned int T_S_START = 0x1;
static constexpr unsigned int T_S_END = 0x4;

struct tdc40 {
	int type;
	int tot;
	int flag;
	int ch;
	int tdc;
	int hartbeat;
};

void Rev5(unsigned char *val, unsigned char *rval)
{
	rval[4] = val[0];
	rval[3] = val[1];
	rval[2] = val[2];
	rval[1] = val[3];
	rval[0] = val[4];

	return;
}

int Unpack(unsigned char *data, struct tdc40 *tdc)
{
	tdc->type = (data[4] & 0xf0) >> 4;
	if (tdc->type == T_TDC) {
		tdc->tot  = ((data[4] & 0x07) << 5) | ((data[3] & 0xf8) >> 3);
		tdc->flag = (data[3] & 0x06) >> 1;
		tdc->ch   = ((data[3] & 0x01) << 5) | (data[2] & 0xf8) >> 3;
		tdc->tdc  = ((data[2] & 0x07) << 16) | (data[1] << 8) | data[0];
		tdc->hartbeat = 0;
	} else 
	if ((tdc->type == T_HB) || (tdc->type == T_ERROR)) {
		tdc->tot  = -1;
		tdc->flag = -1;
		tdc->ch   = -1;
		tdc->tdc  = -1;
		tdc->hartbeat = ((data[1] << 8) | data[0]);
	} else {
		tdc->tot  = -1;
		tdc->flag = -1;
		tdc->ch   = -1;
		tdc->tdc  = -1;
		tdc->hartbeat = 0;
	}

	return tdc->type;
}
} //namespace TDC40


namespace TDC64 {

static constexpr unsigned int T_TDC = (0x2c >> 2);
static constexpr unsigned int T_HB =  (0x70 >> 2);
static constexpr unsigned int T_S_START = (0x60 >> 2);
static constexpr unsigned int T_S_END = (0x50 >> 2);

struct tdc64 {
	int type;
	int ch;
	int tot;
	int tdc;
	int flag;
	int spill;
	int hartbeat;
};

int Unpack(uint64_t data, struct tdc64 *tdc)
{
	//unsigned char *cdata = reinterpret_cast<unsigned char *>(&data);

	tdc->type = (data & 0xfc00'0000'0000'0000) >> 58;
	if (tdc->type == T_TDC) {
		tdc->ch       = (data & 0x03f8'0000'0000'0000) >> 51;
		tdc->tot      = (data & 0x0007'ffff'e000'0000) >> 29;
		tdc->tdc      = (data & 0x0000'0000'1fff'ffff);
		tdc->spill    = -1;
		tdc->hartbeat = -1;
	} else
	if (tdc->type == T_HB) {
		tdc->ch       = -1;
		tdc->tot      = -1;
		tdc->tdc      = -1;
		tdc->flag     = (data & 0x03ff'0000'0000'0000) >> 48;
		tdc->spill    = (data & 0x0000'ff00'0000'0000) >> 40;
		tdc->hartbeat = (data & 0x0000'00ff'ff00'0000) >> 24;
	} else
	if ((tdc->type == T_S_START) || (tdc->type == T_S_END)) {
		tdc->ch       = -1;
		tdc->tot      = -1;
		tdc->tdc      = -1;
		tdc->flag     = (data & 0x03ff'0000'0000'0000) >> 48;
		tdc->spill    = (data & 0x0000'ff00'0000'0000) >> 40;
		tdc->hartbeat = (data & 0x0000'00ff'ff00'0000) >> 24;
	} else {
		tdc->ch       = -1;
		tdc->tot      = -1;
		tdc->tdc      = -1;
		tdc->flag     = -1;
		tdc->spill    = -1;
		tdc->hartbeat = -1;
	}

	return tdc->type;
}

#if 0
int Unpack(uint64_t data, struct tdc64 *tdc)
{
	unsigned char *cdata = reinterpret_cast<unsigned char *>(&data);
	return Unpack(cdata, tdc);
}
#endif

int Unpack(unsigned char *data, struct tdc64 *tdc)
{
	uint64_t *pdata = reinterpret_cast<uint64_t *>(data);
	return Unpack(*pdata, tdc);
}


} //namespace TDC64

namespace TDC64L {

static constexpr unsigned int T_TDC = (0x2c >> 2);
static constexpr unsigned int T_HB =  (0x70 >> 2);
static constexpr unsigned int T_S_START = (0x60 >> 2);
static constexpr unsigned int T_S_END = (0x50 >> 2);

struct tdc64 {
	int type;
	int ch;
	int tot;
	int tdc;
	int flag;
	int spill;
	int hartbeat;
};

int Unpack(uint64_t data, struct tdc64 *tdc)
{
	//unsigned char *cdata = reinterpret_cast<unsigned char *>(&data);

	tdc->type = (data & 0xfc00'0000'0000'0000) >> 58;
	if (tdc->type == T_TDC) {
		tdc->ch       = (data & 0x03f8'0000'0000'0000) >> 51;
		tdc->tot      = (data & 0x0007'f800'0000'0000) >> 43;
		tdc->tdc      = (data & 0x0000'07ff'ff00'0000) >> 24;
		tdc->flag     = -1;
		tdc->spill    = -1;
		tdc->hartbeat = -1;
	} else
	if (tdc->type == T_HB) {
		tdc->ch       = -1;
		tdc->tot      = -1;
		tdc->tdc      = -1;
		tdc->flag     = (data & 0x03ff'0000'0000'0000) >> 48;
		tdc->spill    = (data & 0x0000'ff00'0000'0000) >> 40;
		tdc->hartbeat = (data & 0x0000'00ff'ff00'0000) >> 24;
	} else
	if ((tdc->type == T_S_START) || (tdc->type == T_S_END)) {
		tdc->ch       = -1;
		tdc->tot      = -1;
		tdc->tdc      = -1;
		tdc->flag     = (data & 0x03ff'0000'0000'0000) >> 48;
		tdc->spill    = (data & 0x0000'ff00'0000'0000) >> 40;
		tdc->hartbeat = (data & 0x0000'00ff'ff00'0000) >> 24;
	} else {
		tdc->ch       = -1;
		tdc->tot      = -1;
		tdc->tdc      = -1;
		tdc->flag     = -1;
		tdc->spill    = -1;
		tdc->hartbeat = -1;
	}

	return tdc->type;
}

} //namespace TDC64L


#if 0
int main(int argc, char* argv[])
{
	static char cbuf[16];
	uint64_t *pdata = reinterpret_cast<uint64_t *>(cbuf);
	struct TDC64::tdc64 tdc;

	while (true) {
		std::cin.read(cbuf, 8);
		//std::cin >> *pdata; 
		if (std::cin.eof()) break;

		int type = TDC64::Unpack(*pdata, &tdc);
		if (type == TDC64::T_TDC) {
			std::cout << "TDC " << std::dec
				<< " CH:" << tdc.ch 
				<< " TOT:" << tdc.tot 
				<< " TDC:" << tdc.tdc 
				<< " : " << std::hex << *pdata
				<< std::endl;
		} else
		if ((type == TDC64::T_HB)
			|| (tdc.type == TDC64::T_S_START)
			|| (tdc.type == TDC64::T_S_END)) {
			if (type ==TDC64::T_HB) std::cout << "HB ";
			if (type ==TDC64::T_S_START) std::cout << "S_STA ";
			if (type ==TDC64::T_S_END) std::cout << "S_END ";
			std::cout  << std::hex
				<< " FLAG: " << tdc.flag
				<< " SPILL: " << tdc.spill
				<< " HERTBEAT: " << tdc.hartbeat
				<< " : " << std::hex << *pdata
				<< std::dec << std::endl;
		} else {
			std::cerr << "Invalid data : "
				<< std::hex << *pdata << std::dec << std::endl;
			break;
		}
	}

	return 0;
}
#endif

#if 0
int main(int argc, char* argv[])
{
	static unsigned char cbuf[16];
	//uint64_t *pdata = reinterpret_cast<uint64_t *>(cbuf);
	struct TDC40::tdc40 tdc;

	std::cin.read(reinterpret_cast<char *>(cbuf), 13);
	while (true) {
		std::cin.read(reinterpret_cast<char *>(cbuf), 5);
		//std::cin >> *pdata; 
		if (std::cin.eof()) break;

		int type = TDC40::Unpack(cbuf, &tdc);
		if (type == TDC40::T_TDC) {
			std::cout << "TDC " << std::dec
				<< " CH:" << std::setw(2) << tdc.ch 
				<< " TOT:" << std::setw(3) << tdc.tot 
				<< " TDC:" << std::setw(7) << tdc.tdc 
				<< " : " << std::hex
				<< std::setw(2) << static_cast<unsigned int>(cbuf[4])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[3])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[2])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[1])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[0])
				<< std::endl;
		} else
		if ((type == TDC40::T_HB)
			|| (tdc.type == TDC40::T_ERROR)
			|| (tdc.type == TDC40::T_S_START)
			|| (tdc.type == TDC40::T_S_END)) {
			if (type ==TDC40::T_HB) std::cout << "HB ";
			if (type ==TDC40::T_ERROR) std::cout << "ERR ";
			if (type ==TDC40::T_S_START) std::cout << "S_STA ";
			if (type ==TDC40::T_S_END) std::cout << "S_END ";
			std::cout  << std::dec
				//<< " FLAG: " << tdc.flag
				//<< " SPILL: " << tdc.spill
				<< " HERTBEAT: " << tdc.hartbeat
				<< " : " << std::hex
				<< std::setw(2) << static_cast<unsigned int>(cbuf[4])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[3])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[2])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[1])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[0])
				<< std::dec << std::endl;
		} else {
			std::cout << "Invalid data : " << std::hex
				<< std::setw(2) << static_cast<unsigned int>(cbuf[4])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[3])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[2])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[1])
				<< std::setw(2) << static_cast<unsigned int>(cbuf[0])
				<< std::dec << std::endl;
			//break;
		}
	}

	return 0;
}
#endif

#if 0
int main(int argc, char* argv[])
{
	struct TDC40::tdc40 tdc;

	unsigned char rawdata[] = {
		0x00, 0x00, 0x80, 0x40, 0xd0,
		0x11, 0x00, 0x80, 0x40, 0xd0,
		0x22, 0x00, 0x80, 0x40, 0xd0,
		0x33, 0x00, 0x80, 0x40, 0xd0,
		0x44, 0x00, 0x80, 0x40, 0xd0,
		0x02, 0x00, 0x00, 0x00, 0xf0		
	};


	for (int i = 0 ; i < 30 ; i += 5) {
		TDC40::Unpack((rawdata + i), &tdc);
		std::cout << " Head: " << std::hex << tdc.type
			<< " TOT:  " << std::hex << tdc.tot
			<< " TYPE: " << std::hex << tdc.flag
			<< " CH:   " << std::hex << tdc.ch
			<< " TDC:  " << std::hex << tdc.tdc
			<< " HB:  " << std::hex << tdc.hartbeat
			<< std::endl;
	}
	

	return 0;
}
#endif
