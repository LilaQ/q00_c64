#include <string>
#include <vector>
#include <iostream>
using namespace std;

struct Entry {
	bool available = false;
	uint8_t next_track;
	uint8_t next_sector;
	string file_type;
	uint8_t start_track;
	uint8_t start_sector;
	string pet_name;
	uint32_t adress_start;
	uint32_t adress_end;
	uint16_t sector_size;
	uint8_t sectors;
};

/*
	Track   Sectors/track   # Sectors   Storage in Bytes
        -----   -------------   ---------   ----------------
         1-17        21            357           7820
        18-24        19            133           7170
        25-30        18            108           6300
        31-40(*)     17             85           6020
                                   ---
                                   683 (for a 35 track image)

  Track #Sect #SectorsIn D64 Offset   Track #Sect #SectorsIn D64 Offset
  ----- ----- ---------- ----------   ----- ----- ---------- ----------
    1     21       0       $00000      21     19     414       $19E00
    2     21      21       $01500      22     19     433       $1B100
    3     21      42       $02A00      23     19     452       $1C400
    4     21      63       $03F00      24     19     471       $1D700
    5     21      84       $05400      25     18     490       $1EA00
    6     21     105       $06900      26     18     508       $1FC00
    7     21     126       $07E00      27     18     526       $20E00
    8     21     147       $09300      28     18     544       $22000
    9     21     168       $0A800      29     18     562       $23200
   10     21     189       $0BD00      30     18     580       $24400
   11     21     210       $0D200      31     17     598       $25600
   12     21     231       $0E700      32     17     615       $26700
   13     21     252       $0FC00      33     17     632       $27800
   14     21     273       $11100      34     17     649       $28900
   15     21     294       $12600      35     17     666       $29A00
   16     21     315       $13B00      36(*)  17     683       $2AB00
   17     21     336       $15000      37(*)  17     700       $2BC00
   18     19     357       $16500      38(*)  17     717       $2CD00
   19     19     376       $17800      39(*)  17     734       $2DE00
   20     19     395       $18B00      40(*)  17     751       $2EF00
*/

struct D64Parser {

	string FILE_TYPE[0x100];
	unsigned char data[0xf0000];
	string filename;
	string diskname;
	std::vector<Entry> entries;
	const uint32_t STARTS[41] = {
		0,
		0x00000, 0x01500, 0x02a00, 0x03f00, 0x05400, 0x06900, 0x07e00, 0x09300,
		0x0a800, 0x0bd00, 0x0d200, 0x0e700, 0x0fc00, 0x11100, 0x12600, 0x13b00,
		0x15000, 0x16500, 0x17800, 0x18b00, 0x19e00, 0x1b100, 0x1c400, 0x1d700,
		0x1ea00, 0x1fc00, 0x20e00, 0x22000, 0x23200, 0x24400, 0x25600, 0x26700,
		0x27800, 0x28900, 0x29a00, 0x2ab00, 0x2bc00, 0x2cd00, 0x2de00, 0x2ef00
	};

	void init(string f) {

		FILE_TYPE[0x80] = "DEL";
		FILE_TYPE[0x81] = "SEQ";
		FILE_TYPE[0x82] = "PRG";
		FILE_TYPE[0x83] = "USR";
		FILE_TYPE[0x84] = "REL";
		entries.clear();
		filename = f;
		FILE* file = fopen(filename.c_str(), "rb");
		int pos = 0;
		while (fread(&data[pos], 1, 1, file)) {
			pos++;
		}
		fclose(file);

		//	init all available lines
		uint32_t base_dir = 0x16600;
		for (int i = 0; i < 0x1200; i += 0x20) {
			Entry entry;
			entry.next_track = data[base_dir + i];
			entry.next_sector = data[base_dir + i + 1];
			entry.file_type = FILE_TYPE[data[base_dir + i + 2]];
			entry.start_track = data[base_dir + i + 3];
			entry.start_sector = data[base_dir + i + 4];
			entry.pet_name;
			for (int j = 5; j < 0x15; j++) {
				entry.pet_name += data[base_dir + i + j];
			}
			entry.sector_size = data[base_dir + i + 0x1e] + (data[base_dir + i + 0x1f] * 256);
			entry.available = (entry.file_type != "");
			
			entry.sectors = 21;
			if (entry.start_track > 17 && entry.start_track < 25)
				entry.sectors = 19;
			else if (entry.start_track > 24 && entry.start_track < 31)
				entry.sectors = 18;
			else if (entry.start_track > 30)
				entry.sectors = 17;
			entry.adress_start = STARTS[entry.start_track] + entry.start_sector * 256;
			entry.adress_end = entry.adress_start + entry.sector_size * 256;
			entries.push_back(entry);
		}
		diskname = "";
		for (uint8_t i = 0x90; i < 0xa0; i++) {
			diskname += data[STARTS[18] + i];
		}

	}

	std::vector<uint8_t> dirList() {
		uint8_t lc = 0x1f;
		uint8_t lh = 0x08;
		std::vector<uint8_t> r{
			0x1f, 0x08, 0x00
		};
		r.push_back(0x00);												//	HEADLINE 0
		r.push_back(0x12);												//	HEADLINE BOLD ?
		r.push_back(0x22);												//	"
		for (int i = 0; i < diskname.size(); i++) {
			r.push_back(diskname.at(i));
		}
		r.push_back(0x22);												//	"
		for (int i = 0; i < 24 - diskname.size() - 2; i++) {
			r.push_back(0x20);											//	FILLING SPACES
		}
		r.push_back(0x00);
		for (int i = 0; i < entries.size(); i++) {
			if (entries[i].available) {
				if ((lc + 0x20) > 0xff)
					lh++;
				lc += 0x20;
				r.push_back(lc);										//	Line separation
				r.push_back(lh);										//	Line separation
				r.push_back(entries[i].sector_size);					//	SIZE
				r.push_back(0x00);
				string f = std::to_string(entries[i].sector_size);
				for (int i = 0; i < 4 - f.size(); i++) {
					r.push_back(0x20);									//	SPACE
				}
				r.push_back(0x22);										//	"
				for (int j = 0; j < entries[i].pet_name.size(); j++) {
					r.push_back(entries[i].pet_name.at(j));				//	NAME
				}
				r.push_back(0x22);										//	"
				r.push_back(0x20);										//	SPACE
				for (int j = 0; j < entries[i].file_type.size(); j++) {
					r.push_back(entries[i].file_type.at(j));			//	FILE_EXT
				}
				for (int j = 0; j < 26 - entries[i].pet_name.size() - 5 - (4 - f.size()); j++) {
					r.push_back(0x20);									//	FILLING SPACES
				}
				r.push_back(0x00);
			}
		}
		r.push_back(0x1d);
		return r;
	}

	std::vector<uint8_t> getData(uint8_t row_id) {
		std::vector<uint8_t> ret(entries.at(row_id).sector_size * 0x100);
		int c = 0;
		uint32_t next_track = entries.at(row_id).start_track;
		uint32_t next_sector = entries.at(row_id).start_sector;
		while(c < entries.at(row_id).sector_size) {
			uint32_t a_adr = STARTS[next_track] + next_sector * 256;
			for (int i = 0; i < 254; i++) {
				ret[c * 254 + i] = data[a_adr + i + 2];
			}
			next_track = data[a_adr];
			next_sector = data[a_adr + 1];
			c++;
		}
		return ret;
	}

	bool filenameExists(string f) {
		for (int i = 0; i < entries.size(); i++) {
			cout << entries.at(i).pet_name << " - " << f << "\n";
			if (entries.at(i).pet_name == f)
				return true;
		}
		return false;
	}

	std::vector<uint8_t> getDataByFilename(string f) {
		Entry selected;
		for (int i = 0; i < entries.size(); i++) {
			if (entries.at(i).pet_name == f)
				selected = entries.at(i);
		}
		int c = 0;
		std::vector<uint8_t> ret(selected.sector_size * 0x100);
		uint32_t next_track = selected.start_track;
		uint32_t next_sector = selected.start_sector;
		while (c < selected.sector_size) {
			uint32_t a_adr = STARTS[next_track] + next_sector * 256;
			for (int i = 0; i < 254; i++) {
				ret[c * 254 + i] = data[a_adr + i + 2];
			}
			next_track = data[a_adr];
			next_sector = data[a_adr + 1];
			c++;
		}
		return ret;
	}

	void printAll() {
		cout << "\t\t" << diskname << "\n";
		for (int i = 0; i < entries.size(); i ++) {
			if (entries.at(i).available) {
				cout << entries.at(i).file_type << "\t";
				cout << hex << +entries.at(i).start_track;
				cout << "/";
				cout << hex << +entries.at(i).start_sector << "\t";
				cout << entries.at(i).pet_name << "\t" << dec << entries.at(i).sector_size;
				cout << "\t" << hex << entries.at(i).adress_start << " -> " << hex << entries.at(i).adress_end << "\n";
			}
		}
	}
};