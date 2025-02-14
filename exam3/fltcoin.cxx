/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *									      *
 *	      This software is distributed under the terms of the	     *
 *	      GNU Lesser General Public Licence (LGPL) version 3,	     *
 *		  copied verbatim in the file "LICENSE"		       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

#include <unordered_map>
#include <unordered_set>

#include "MessageUtil.h"

#include "HulStrTdcData.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"

#include "ktimer.cxx"
#include "trigger.cxx"


//std::atomic<int> gQdepth = 0;

namespace bpo = boost::program_options;

struct FltCoin : fair::mq::Device
{
	struct OptionKey {
		static constexpr std::string_view InputChannelName   {"in-chan-name"};
		static constexpr std::string_view OutputChannelName  {"out-chan-name"};
		static constexpr std::string_view DataSupress        {"data-supress"};
		static constexpr std::string_view PollTimeout        {"poll-timeout"};
	};

	FltCoin()
	{
		// register a handler for data arriving on "data" channel
		//OnData("in", &FltCoin::HandleData);

		fTrig = new Trigger();
		fKt1 = new KTimer(1000);
		fKt2 = new KTimer(1000);
		fKt3 = new KTimer(1000);
	}

	void InitTask() override;
#if 0
	void InitTask() override
	{
		using opt = OptionKey;

		// Get the fMaxIterations value from the command line options (via fConfig)
		fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
		fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
		fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());
		LOG(info) << "InitTask: Input Channel : " << fInputChannelName
			<< " Output Channel : " << fOutputChannelName;

		//OnData(fInputChannelName, &FltCoin::HandleData);
	}
#endif

#if 0
	bool HandleData(fair::mq::MessagePtr& msg, int)
	{
		LOG(info) << "Received: \""
			<< std::string(static_cast<char*>(msg->GetData()), msg->GetSize())
			<< "\"";

		if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
			LOG(info) << "Configured maximum number of iterations reached."
				<< " Leaving RUNNING state.";
			return false;
		}

		// return true if you want the handler to be called again
		// (otherwise return false go to the Ready state)
		return true;
	}
#endif

	
	bool ConditionalRun() override;
	void PostRun() override;

	bool CheckData(fair::mq::MessagePtr&);

private:
	int IsHartBeat(uint64_t, uint32_t);

	std::string fInputChannelName;
	std::string fOutputChannelName;
	std::string fName;

	int fNumDestination {0};
	uint32_t fDirection {0};
	int fPollTimeoutMS  {0};

	uint32_t fId {0};
	Trigger *fTrig;
	bool fIsDataSupress = true;
	KTimer *fKt1;
	KTimer *fKt2;
	KTimer *fKt3;
};


void FltCoin::InitTask()
{
	using opt = OptionKey;

	fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
	fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());

	LOG(info) << "InitTask: Input Channel : " << fInputChannelName
		<< " Output Channel : " << fOutputChannelName;

	fNumDestination = GetNumSubChannels(fOutputChannelName);
	fPollTimeoutMS  = std::stoi(fConfig->GetProperty<std::string>(opt::PollTimeout.data()));

        fName = fConfig->GetProperty<std::string>("id");
        std::istringstream ss(fName.substr(fName.rfind("-") + 1));
        ss >> fId;

	std::string sIsDataSupress = fConfig->GetValue<std::string>(opt::DataSupress.data());
	if (sIsDataSupress == "true") {
		fIsDataSupress = true;
	} else {
		fIsDataSupress = false;
	}
	LOG(info) << "InitTask: DataSupress : " << fIsDataSupress;

	//fTrig->SetTimeRegion(1024 * 256);
	fTrig->SetTimeRegion(1024 * 128);
	fTrig->ClearEntry();
	fTrig->Entry(0xc0a802a8, 2, 0);
	fTrig->Entry(0xc0a802a8, 4, 0);
	//fTrig->Entry(0xc0a802a8, 6, 0);
	//fTrig->Entry(0xc0a802a8, 8, 0);

}

bool FltCoin::CheckData(fair::mq::MessagePtr &msg)
{
	unsigned int msize = msg->GetSize();
	unsigned char *pdata = reinterpret_cast<unsigned char *>(msg->GetData());
	uint64_t msg_magic = *(reinterpret_cast<uint64_t *>(pdata));

	static int fe_type = 0;

	std::cout << "#Msg Top(8B): " << std::hex << msg_magic
		<< " Size: " << std::dec << msize << std::endl;

	if (msg_magic == TimeFrame::Magic) {
		TimeFrame::Header *ptf = reinterpret_cast<TimeFrame::Header *>(pdata);
		std::cout << "#TF Header "
			<< std::hex << std::setw(16) << std::setfill('0') <<  ptf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  ptf->timeFrameId
			<< " Nsource: " << std::setw(8) << std::setfill('0') <<  ptf->numSource
			<< " len: " << std::dec <<  ptf->length
			<< std::endl;
		#if 0
		for (unsigned int i = 0 ; i < ptf->length ; i++) {
			if ((i % 16) == 0) {
				std::cout << std::endl
					<< "#" << std::setw(8) << std::setfill('0')
					<< i << " : ";
			}
			std::cout << " "
				<< std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<unsigned int>(pdata[i]);
		}
		std::cout << std::endl;
		#endif
		
	} else if (msg_magic == SubTimeFrame::Magic) {
		SubTimeFrame::Header *pstf
				= reinterpret_cast<SubTimeFrame::Header *>(pdata);
		std::cout << "#STF Header "
			<< std::hex << std::setw(8) << std::setfill('0') <<  pstf->magic
			<< " id: " << std::setw(8) << std::setfill('0') <<  pstf->timeFrameId
			<< " Type: " << std::setw(8) << std::setfill('0') <<  pstf->Type
			<< " FE: " << std::setw(8) << std::setfill('0') <<  pstf->FEMId
			<< std::endl << "# "
			<< " len: " << std::dec <<  pstf->length
			<< " nMsg: " << std::dec <<  pstf->numMessages
			<< std::endl << "# "
			<< " Ts: " << std::dec << pstf->time_sec
			<< " Tus: " << std::dec << pstf->time_usec
			<< std::endl;

		fe_type = pstf->Type;

		//// toriaezu debug no tameni ireru. atodekesukoto
		//fe_type = 1;
		//pstf->Type = 1;
		//pstf->FEMId = 1234;
		////

	} else {
	       #if 1
		for (unsigned int j = 0 ; j < msize ; j += 8) {
			std::cout << "# " << std::setw(8) << j << " : "
				<< std::hex << std::setw(2) << std::setfill('0')
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 7]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 6]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 5]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 4]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 3]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 2]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 1]) << " "
				<< std::setw(2) << static_cast<unsigned int>(pdata[j + 0]) << " : ";

			if ((pdata[j + 7] & 0xfc) == (TDC64H::T_TDC << 2)) {
				std::cout << "TDC ";
				uint64_t *dword = reinterpret_cast<uint64_t *>(&(pdata[j]));
				if (fe_type == SubTimeFrame::TDC64H) {
					struct TDC64H::tdc64 tdc;
					TDC64H::Unpack(*dword, &tdc);
					std::cout << "H :" 
						<< " CH: " << std::dec << std::setw(3) << tdc.ch
						<< " TDC: " << std::setw(7) << tdc.tdc << std::endl;
				} else
				if (fe_type == SubTimeFrame::TDC64L) {
					struct TDC64L::tdc64 tdc;
					TDC64L::Unpack(*dword, &tdc);
					std::cout << "L :" 
						<< " CH: " << std::dec << std::setw(3) << tdc.ch
						<< " TDC: " << std::setw(7) << tdc.tdc << std::endl;
				} else {
					std::cout << "UNKNOWN Type :0x" << std::hex << fe_type << std::endl;
				}

			} else if ((pdata[j + 7] & 0xfc) == (TDC64H::T_HB << 2)) {
				std::cout << "Hart beat" << std::endl;
			} else if ((pdata[j + 7] & 0xfc) == (TDC64H::T_S_START << 2)) {
				std::cout << "SPILL Start" << std::endl;
			} else if ((pdata[j + 7] & 0xfc) == (TDC64H::T_S_END << 2)) {
				std::cout << "SPILL End" << std::endl;
			} else {
				std::cout << std::endl;
			}
		}
		#else
		std::cout << "#Unknown Header " << std::hex << msg_magic << std::endl;
		#endif
	}


	return true;
}

//int FltCoin::Trigger(FairMQParts &inParts, FairMQPatts &outParts)
//{
//	return 0;
//}

int FltCoin::IsHartBeat(uint64_t val, uint32_t type)
{
	int hbflag = -1;
	int hbframe = -1;
	if (type == SubTimeFrame::TDC64H) {
		struct TDC64H::tdc64 tdc;
		if (TDC64H::Unpack(val, &tdc) == TDC64H::T_HB) {
			hbframe = tdc.hartbeat;
			hbflag = tdc.flag;
		}
	} else
	if (type == SubTimeFrame::TDC64L) {
		struct TDC64L::tdc64 tdc;
		if (TDC64L::Unpack(val, &tdc) == TDC64L::T_HB) {
			hbframe = tdc.hartbeat;
			hbflag = tdc.flag;
		}
	} else {
		std::cout << "Unknown device : " << std::hex << type << std::endl;
	}

	if (hbflag > 0) {
		if ((hbflag & 0x200) == 0x200) std::cout << "#E HB Data lost" << std::endl;
		if ((hbflag & 0x100) == 0x100) std::cout << "#E HB Data confiliction" << std::endl;
		if ((hbflag & 0x080) == 0x080) std::cout << "#E HB LFN mismatch" << std::endl;
		if ((hbflag & 0x040) == 0x040) std::cout << "#E HB GFN mismatch" << std::endl;
	}

	return hbframe;
}

bool FltCoin::ConditionalRun()
{
	//Receive
	FairMQParts inParts;

	FairMQMessagePtr msg_header(fTransportFactory->CreateMessage());
	struct Filter::Header fltheader;
	struct TimeFrame::Header *i_tfHeader;
	std::chrono::system_clock::time_point sw_start, sw_end;

	if (Receive(inParts, fInputChannelName, 0, 1000) > 0) {
		assert(inParts.Size() >= 2);
		if (fKt1->Check()) {
			std::cout << "#Nmsg: " << std::dec << inParts.Size() << std::endl;
		}

		sw_start = std::chrono::system_clock::now();

		struct DataBlock {
			uint32_t FEMId;
			uint32_t Type;
			int HBFrame;
			bool is_HB;
			int msg_index;
			int nTrig;
		};

		std::vector<struct DataBlock> blocks;
		std::vector< std::vector<struct DataBlock> > block_map;

		//std::vector<int> hbblocks;
		//std::vector< std::vector<int> > hbblock_map;
		std::vector<struct SubTimeFrame::Header> stf;

		uint64_t femid = 0;
		uint64_t devtype = 0;
		//int hbframe = 0;
		int ifem = 0;


		std::vector<bool> flag_sending;

		//for(auto& vmsg : inParts) {
		for(int i = 0 ; i < inParts.Size() ; i++) {
			flag_sending.push_back(true);
			#if 0
			CheckData(inParts.At(i));
			#endif

			auto tfHeader = reinterpret_cast<struct TimeFrame::Header *>(inParts[i].GetData());
			auto stfHeader = reinterpret_cast<struct SubTimeFrame::Header *>(inParts[i].GetData());
			struct DataBlock dblock;

			if (tfHeader->magic == TimeFrame::Magic) {
				//iblock = 0;
				ifem = -1;
				stf.clear();
				stf.resize(0);
				i_tfHeader = tfHeader;
			} else
			if (stfHeader->magic == SubTimeFrame::Magic) {
				femid = stfHeader->FEMId;
				devtype = stfHeader->Type;
				if (blocks.size() > 0) block_map.push_back(blocks);
				stf.push_back(*stfHeader);
				//if (hbblocks.size() > 0) hbblock_map.push_back(hbblocks);
				//iblock = 0;
				dblock.is_HB = false;
				dblock.msg_index = -1;
				blocks.clear();
				blocks.resize(0);
				ifem++;
			} else {
				// make block map;

				uint64_t *data = reinterpret_cast<uint64_t *>(inParts[i].GetData());

				int hbframe = IsHartBeat(data[0], devtype);

				//std::cout << "#DDD msg " << std::dec << i << ": "
				//	<< " HBframe: " << hbframe << std::endl;

				if (hbframe < 0) {
					dblock.FEMId = femid;
					dblock.Type = devtype;
					dblock.is_HB = false;
					dblock.msg_index = i;
					dblock.nTrig = 0;
					dblock.HBFrame = 0;
				} else {
					//data ga nakattatokimo push_back

					dblock.HBFrame = hbframe;
					//hbblocks.push_back(iblock);

					dblock.FEMId = femid;
					dblock.Type = devtype;
					dblock.is_HB = true;
					dblock.msg_index = i;
					dblock.nTrig = 0;
				}
				blocks.push_back(dblock);
				//iblock++;
			}
		} /// end of the for loop
		block_map.push_back(blocks);
		//hbblock_map.push_back(hbblocks);

		#if 0
		int nblock = blocks.size();
		std::cout << "#D bloack_map.size: " << block_map.size() << std::endl;
		for (auto& blk : block_map) {
			std::cout << "#D block " << blk.size() << " / ";
			for (auto& b : blk) {
				std::cout << " " << b.msg_index << " is_HB:" << b.is_HB;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		#endif

		//std::cout << "blocks: " << nblock << std::endl;
		int totalhits = 0;
		for (size_t i = 0 ; i < blocks.size() ; i++) {

			fTrig->CleanUpTimeRegion();

			/// mark Hits
			for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
				struct DataBlock *dbl = &block_map[iifem][i];
				uint64_t vfemid = dbl->FEMId;
				//int mindex = dbl->msg_index;
				//if (mindex > 0) {
				if (!(dbl->is_HB)) {
					int mindex = dbl->msg_index;

					#if 0
					std::cout << "#D1 Mark FEM ID: "
					<< std::hex << vfemid
					<< " Type: " << dbl->Type
					<< " HBFrame: " << std::hex
						<< dbl->HBFrame << std::endl;
					std::cout << "#D1 mindex: "
					<< std::dec << mindex
					<< " vfemid: " << vfemid
					<< " type: " << dbl->Type
					<< std::endl;
					std::cout << "#D1 inParts.size: "
					<< inParts[mindex].GetSize()
					<< std::endl;
					#endif

					fTrig->Mark(
						reinterpret_cast<unsigned char *>(inParts[mindex].GetData()),
						inParts[mindex].GetSize(),
						vfemid, dbl->Type);
				}
			}

			#if 0
			uint32_t *tr = fTrig->GetTimeRegion();
			std::cout << "####DDDD Hit TimeRegion: ";
			for (uint32_t ii = 0 ; ii < fTrig->GetTimeRegionSize() ; ii++) {
				if (tr[ii] != 0) {
				std::cout << " " << std::dec << i << ":"
					<< std::hex << std::setw(4) << std::setfill('0')
					<< tr[ii];
				}
			}
			std::cout << std::endl;
			#endif

			#if 0
			std::cout << "# HB: " << std::dec << i;
			for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
				struct DataBlock *dbl = &block_map[iifem][i];
				//uint64_t vfemid = dbl->FEMId;
				uint64_t vhbframe = dbl->HBFrame;
				//std::cout << "# HB: " << std::dec << i
				//<< " FEM: " << std::hex << vfemid
				std::cout << " " << std::dec << vhbframe;
			}
			std::cout << std::endl;
			#endif

			/// check coincidence
			std::vector<uint32_t> *hits = fTrig->Scan();
			int nhits = hits->size();

			#if 0
			if (nhits > 0) {
				std::cout << "#D1 Hits : " << hits->size() << " ";
				for (unsigned int ii = 0 ; ii < hits->size() ; ii++) {
					std::cout << " " << std::dec << (*hits)[ii];
					if (ii > 10) {
						std::cout << "...";
						break;
					}
				}
				std::cout << std::endl;
			}
			#endif

			//std::cout << std::dec;
			//std::cout << "#D flag_sending.size() "
			//	<< flag_sending.size() << std::endl;
			//std::cout << "#D block_map.size() "
			//	<< block_map.size() << std::endl;

			for (size_t iifem = 0 ; iifem < block_map.size() ; iifem++) {
				block_map[iifem][i].nTrig = nhits;
				if (nhits == 0) {
					int mindex = block_map[iifem][i].msg_index;
					bool is_HB = block_map[iifem][i].is_HB;

					//std::cout << "#D msg_index: " << mindex << std::end;;
					//std::cout << "#D mindex: " << mindex
					//	<< " h: " << is_HB
					//	<< " t: " << block_map[iifem][i].Type
					//	<< " i: " << std::hex << block_map[iifem][i].FEMId
					//	<< std::dec << std::endl;;

					if ((mindex > 0) && (! is_HB)) {
						flag_sending[mindex] = false;
						flag_sending[mindex + 1] = false;
					}

					///// HB wo otoshitemiru
					if ((mindex > 0) && is_HB) {
						flag_sending[mindex] = false;
					}
				} else {
					//int mindex = block_map[iifem][i].msg_index;
					//bool is_HB = block_map[iifem][i].is_HB;
					//std::cout << "#D H mindex: " << mindex
					//	<< " h: " << is_HB
					//	<< " t: " << block_map[iifem][i].Type
					//	<< " i: " << std::hex << block_map[iifem][i].FEMId
					//	<< " Nhits: " << std::dec << nhits
					//	<< std::endl;;
				}
			}

			totalhits += nhits;
			//std::cout << "# block: " << i << " nhits: " << nhits << std::endl;

		}

		#if 1
		if (fKt2->Check()) {
			//std::cout << "#block_map size: " << block_map.size() << std::endl;
			for (unsigned int i = 0 ; i < block_map.size() ; i++) {
				std::cout << "#HBFrame: " << i << "/";
				for (unsigned int j = 0 ; j < block_map[i].size(); j++) {
					std::cout << "  " << j << ":"
						<< std::setw(5) << block_map[i][j].HBFrame;
				}
				std::cout << std::endl;
			}
			//std::cout << std::endl;

			//if (totalhits > 0) {
			//	std::cout << "#D TotalHits: " << totalhits;
			//	std::cout << " Flag: ";
			//	for (const auto& v : flag_sending) std::cout << " " << v; 
			//	std::cout << std::endl;
			//}
		}
		#endif


		#if 0
		gQdepth++;
		FairMQMessagePtr fltheadermsg(
			NewMessage(
			reinterpret_cast<char *>(&fltHeader),
			sizeof(fltHeader),
			[](void* pdata, void*) {
				char *p = reinterpret_cast<char *>(pdata);
				delete p;
				//gQdepth--;
			},
			nullptr));
		outParts.AddPart(std::move(fltheadermsg));
		#endif


		#if 0
		auto tfHeader = reinterpret_cast<TimeFrame::Header*>(inParts.At(0)->GetData());
		auto stfHeader = reinterpret_cast<SubTimeFrame::Header*>(inParts.At(0)->GetData());
		auto stfId     = stfHeader->timeFrameId;
		#endif

		FairMQParts outParts;

		sw_end = std::chrono::system_clock::now();
		uint32_t elapse = std::chrono::duration_cast<std::chrono::microseconds>(
			sw_end - sw_start).count();
		if (fKt3->Check()) {
		//if (totalhits > 0) {
			std::cout << "#Elapse: " << std::dec << elapse << " us"
				<< " Hits: " << totalhits << std::endl;
		}


		//Modify SubTimeFrameHeader, TimeFrameHeader
		uint32_t tf_len = 0;
		if (fIsDataSupress) {
			for (int ii = 0 ; ii < inParts.Size() ; ii++) {
				auto stfh = reinterpret_cast<struct SubTimeFrame::Header *>
					(inParts[ii].GetData());
				if (stfh->magic == SubTimeFrame::Magic) {
					uint32_t len_stf = 0;
					int kk = ii + 1;
					for (int jj = ii + 1 ; jj < inParts.Size() ; jj++) {
						auto sstf =
							reinterpret_cast<struct SubTimeFrame::Header *>
							(inParts[jj].GetData());
						if (sstf->magic == SubTimeFrame::Magic) {
							kk = jj;
							break;
						} else
						if (flag_sending[jj]) {
							len_stf += inParts[jj].GetSize();
						}
					}
					if (len_stf == 0 ) {
						flag_sending[ii] = false;
					} else {
						stfh->length
							= len_stf
							+ sizeof(struct SubTimeFrame::Header);
					}
					ii = kk - 1;
				}
			}

			// TimeFrameHeader
			for (int ii = 0 ; ii < inParts.Size() ; ii++) {
				if (flag_sending[ii] || (! fIsDataSupress)) {
					tf_len += (inParts.AtRef(ii)).GetSize();
				}
			}
			i_tfHeader->length = tf_len;
		} else {
			tf_len = i_tfHeader->length;
		}

		#if 0
		//if (totalhits > 0) {
			std::cout << "#DD TotalHits: " << totalhits;
			std::cout << " Flag: ";
			for (const auto& v : flag_sending) std::cout << v; 
			std::cout << std::endl;
		//}
		#endif
		
		//make header message

		//FilterHeader
		auto tp = std::chrono::system_clock::now() ;
		auto d = tp.time_since_epoch();
		uint64_t sec = std::chrono::duration_cast
			<std::chrono::seconds>(d).count();
		uint64_t usec = std::chrono::duration_cast
			<std::chrono::microseconds>(d).count();

		//uint64_t dlen = 0;
		//for (int ii = 0 ; ii < inParts.Size() ; ii++) {
		//	if (flag_sending[ii] || (! fIsDataSupress)) {
		//		dlen += (inParts.AtRef(ii)).GetSize();
		//	}
		//}
		//dlen += sizeof(struct Filter::Header);
		uint64_t flt_len = tf_len + sizeof(struct Filter::Header);	
		
		auto fltHeader = std::make_unique<struct Filter::Header>();
		fltHeader->magic = Filter::Magic;
		fltHeader->length = flt_len;
		fltHeader->numTrigs = totalhits;
		fltHeader->workerId = fId;
		fltHeader->elapseTime = elapse;
		fltHeader->processTime.tv_sec = sec;
		fltHeader->processTime.tv_usec = usec;

		outParts.AddPart(MessageUtil::NewMessage(*this, std::move(fltHeader)));

		//Copy
		unsigned int msg_size = inParts.Size();
		for (unsigned int ii = 0 ; ii < msg_size ; ii++) {
			if (flag_sending[ii] || (! fIsDataSupress)) {
				FairMQMessagePtr msgCopy(fTransportFactory->CreateMessage());
				msgCopy->Copy(inParts.AtRef(ii));
				outParts.AddPart(std::move(msgCopy));
			}
		}

		//std::cout << "#DDD outParts.Size: " << outParts.Size() << std::endl;
	
		//Send
		#if 0
		while (Send(outParts, fOutputChannelName) < 0) {
			// timeout
			if (GetCurrentState() != fair::mq::State::Running) {
				LOG(info) << "Device is not RUNNING";
				break;
			}
			LOG(error) << "Failed to queue output-channel";
		}
		#else
		auto poller = NewPoller(fOutputChannelName);
		while (!NewStatePending()) {
			auto direction = (fDirection++) % fNumDestination;
			poller->Poll(fPollTimeoutMS);
			if (poller->CheckOutput(fOutputChannelName, direction)) {
				if (Send(outParts, fOutputChannelName, direction) > 0) {
					// successfully sent
					break;
				} else {
					LOG(error) << "Failed to queue output-channel";
				}
			}
			if (fNumDestination==1) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		#endif
	}

	return true;
}

void FltCoin::PostRun()
{
	LOG(info) << "Post Run";
	return;
}


#if 0
bool FltCoin::HandleData(fair::mq::MessagePtr& msg, int val)
{
	(void)val;
	#if 0
	LOG(info) << "Received: \""
		<< std::string(static_cast<char*>(msg->GetData()), msg->GetSize())
		<< "\"";
	#endif

	#if 0
	LOG(info) << "Received: " << msg->GetSize() << " : " << val;
	#endif

	if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
		LOG(info) << "Configured maximum number of iterations reached."
			<< " Leaving RUNNING state.";
		return false;
	}

	CheckData(msg);
	

	// return true if you want the handler to be called again
	// (otherwise return false go to the Ready state)
	return true;
}
#endif


void addCustomOptions(bpo::options_description& options)
{
	using opt = FltCoin::OptionKey;

	options.add_options()
		//("max-iterations", bpo::value<uint64_t>()->default_value(0),
		//"Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)")
		(opt::InputChannelName.data(),
			bpo::value<std::string>()->default_value("in"),
			"Name of the input channel")
		(opt::OutputChannelName.data(),
			bpo::value<std::string>()->default_value("out"),
			"Name of the output channel")
		(opt::DataSupress.data(),
			bpo::value<std::string>()->default_value("true"),
			"Data supression enable")
		(opt::PollTimeout.data(), 
			bpo::value<std::string>()->default_value("1"),
			"Timeout of polling (in msec)")
    		;

}


std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
	return std::make_unique<FltCoin>();
}
