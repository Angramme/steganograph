//#define _CRT_SECURE_NO_DEPRECATE // for c-style file I/O

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <stdlib.h>


std::unordered_map<std::string, std::string> load_config(const char* file) {
	using namespace std;

	unordered_map<string, string> DATA;

	ifstream configF(file);
	if (configF.is_open()) {
		char output[100];
		for (unsigned i = 0; !configF.eof() && i < 10; i++) {
			configF >> output;
			cout << output;

			string key, val;
			bool mode = 0;
			for (unsigned j = 0; output[j] != '\0'; j++) {
				if (output[j] == ':') {
					mode = 1;
				}else if (!mode) {
					if(output[j] != ' ') key += output[j];
				}else {
					if(output[j] != ' ') val += output[j];
				}
			}
			if (!mode) {
				cout << " <- invalid expresion! [probably a space or no : keyword]";
			}else {
				DATA[key] = val;
			}

			cout << endl;
		}
	}
	else {
		std::cout << "impossible to open the file!" << std::endl;
	}
	configF.close();

	return DATA;
}

bool ReadBytes(char const* filename, std::vector<char>& buffer)
{
	// C-style file I/O is faster than C++, WAY FASTER!
	using namespace std;

	FILE *fptr;
	auto errorcode = fopen_s(&fptr, filename, "rb");
	if (errorcode != 0) {
		cout << "could not open file: " << filename << "! fopen_s error code value is: " << errorcode << "! aborting operation!" << endl;
		return 0;
	}
	fseek(fptr, 0, SEEK_END);
	long filelen = ftell(fptr);
	rewind(fptr);

	buffer.resize(filelen + 1);
	fread(buffer.data(), filelen, 1, fptr);
	fclose(fptr);

	return 1;
}

bool SaveFile(std::string name, std::vector<char> buffer) {
	std::ofstream ofs(name, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
	if (!ofs.is_open()) {
		std::cout << "error opening output file" << std::endl;
		return 0;
	}

	ofs.write(buffer.data(), buffer.size());

	ofs.close();

	return true;
}

void insert(char* target, const char* data, const unsigned data_length, const unsigned step, const char key = 0) {
	for (unsigned i = 0; i < data_length * step; i += step) {
		char& t1 = target[i*2];
		char& t2 = target[i*2 + 1];
		char d = data[i/step] ^ key;

		t1 &= 0b11110000;
		t2 &= 0b11110000;

		t1 |= (d >> 4) & 0b00001111;
		t2 |= d & 0b00001111;
	}
}

void extract(const char* target, char* output, const unsigned output_length, const unsigned step, const char key = 0) {
	for (unsigned i = 0; i < output_length * step; i += step) {
		const char& t1 = target[i * 2];
		const char& t2 = target[i * 2 + 1];
		char& d = output[i/step];

		d = 0;
		d |= (t1 << 4) & 0b11110000;
		d |= t2 & 0b00001111;

		d ^= key;
	}
}

bool Encode(std::vector<char>& bitmap, std::vector<char>& data, std::unordered_map<std::string, std::string>& config) {
	using namespace std;
	
	if (bitmap[0] != 0x42) {
		cout << "bitmap format header incorrect or not present! aborting operation!" << endl;
		return false;
	}

	//data header
	unsigned ofst = 0x35 + 1; // bitmap header last byte location + 1
	
	{ // signal
		const char* headerSignal = "OK19";
		insert(&bitmap[ofst], headerSignal, 4, 1);
		ofst += 4 * 2;
	}
	const char ENCRYPT_KEY = ( (config["encrypt"] == "1") ? bitmap[0x3] : 0 ) ^ config["password_char"][0];
	{ // encrypt bool
		const bool encrypt_bool = config["encrypt"] == "1";
		insert(&bitmap[ofst], reinterpret_cast<const char *>(&encrypt_bool), sizeof(bool), 1);
		ofst += sizeof(bool) * 2;

		if (encrypt_bool) {
			cout << "encryption enabled! key is: " << ENCRYPT_KEY << endl;
		}
	}
	{ // size of data
		const size_t data_size = data.size();
		insert(&bitmap[ofst], reinterpret_cast<const char*>(&data_size), sizeof(size_t), 1, ENCRYPT_KEY);
		ofst += sizeof(size_t) * 2;
	}
	{ // size of data files name
		const size_t data_file_name_size = config["data"].length();
		insert(&bitmap[ofst], reinterpret_cast<const char*>(&data_file_name_size), sizeof(size_t), 1, ENCRYPT_KEY);
		ofst += sizeof(size_t) * 2;
	}
	{ // data file name
		insert(&bitmap[ofst], config["data"].data(), config["data"].length() * sizeof(char), 1, ENCRYPT_KEY);
		ofst += config["data"].length() * sizeof(char) * 2;
	}
	// bit offset
		const unsigned bit_offset = (unsigned)std::stoi(config["bit-offset"]) + 1U;
		insert(&bitmap[ofst], reinterpret_cast<const char*>(&bit_offset), sizeof(unsigned), 1, ENCRYPT_KEY);
		ofst += sizeof(unsigned) * 2;
	
	{ // the actual data of the file ( FINALLY! )
		if (ofst + data.size() > bitmap.size()) {
			cout << "Not enough space to encode data: " << bitmap.size() << " < " << ofst + data.size() << endl;
			return 0;
		}

		insert(&bitmap[ofst], data.data(), data.size() * sizeof(char), bit_offset, ENCRYPT_KEY);
	}

	return true;
}

bool Decode(std::vector<char>& bitmap, std::vector<char>& output, std::unordered_map<std::string, std::string>& config) {
	using namespace std;

	//data header
	unsigned ofst = 0x35 + 1; // bitmap header last byte location + 1

	{ // signal
		string signal;
		signal.resize(4);
		extract(&bitmap[ofst], const_cast<char*>(signal.data()), 4, 1);
		ofst += 4 * 2;

		if (signal != "OK19") {
			std::cout << "OK19 header not present! No hidden information. present information: " << signal << std::endl;
			return 0;
		}

		cout << "header: " << signal << endl;
	}


	// encrypt bool
	bool encrypt_bool;
	extract(&bitmap[ofst], reinterpret_cast<char *>(&encrypt_bool), sizeof(bool), 1);
	ofst += sizeof(bool) * 2;

	const char DECRYPT_KEY = ( encrypt_bool ? bitmap[0x3] : 0 ) ^ config["password_char"][0];
	if (encrypt_bool) {
		cout << "encryption was enabled! key is: " << DECRYPT_KEY << endl;
	}


	// size of data
	size_t data_size;
	extract(&bitmap[ofst], reinterpret_cast<char*>(&data_size), sizeof(size_t), 1, DECRYPT_KEY);
	ofst += sizeof(size_t) * 2;
	cout << "data size: " << data_size << endl;

	{
		// size of data files name
		size_t data_file_name_size;
		extract(&bitmap[ofst], reinterpret_cast<char*>(&data_file_name_size), sizeof(size_t), 1, DECRYPT_KEY);
		ofst += sizeof(size_t) * 2;
		cout << "file name length: " << data_file_name_size << endl;

		// data file name
		std::string file_name;
		file_name.resize(data_file_name_size);
		extract(&bitmap[ofst], const_cast<char *>(file_name.data()), data_file_name_size, 1, DECRYPT_KEY);
		ofst += data_file_name_size * 2;
		cout << "file name: " << file_name << endl;

		config["data"] = file_name;
	}
	// bit offset
	unsigned bit_offset = 0;
	extract(&bitmap[ofst], reinterpret_cast<char*>(&bit_offset), sizeof(unsigned int), 1, DECRYPT_KEY);
	ofst += sizeof(unsigned int) * 2;
	cout << "bit offset: " << bit_offset << endl;

	// the actual data of the file ( FINALLY! )
	output.resize(data_size);
	extract(&bitmap[ofst], output.data(), data_size, bit_offset, DECRYPT_KEY);

	return true;
}


int ABORT(const char* message = "") {
	std::cout << "\t<!> ABORT: " << message << std::endl;
	std::cout << "please press enter to finish";
	std::cin.get();
	return 1;
}


int main(int ac, char** av) {
	std::cout << "=== Steganograph v1.0 by Kacper Ozieblowski ===" << std::endl;

	if (ac == 1) {
		std::cout << "Please specify the config file name," << std::endl;
		std::cout << "or create a new one by passing -new as an argument followed by the name of the new file." << std::endl;
		std::cout << "You can also decode hidden data inside .bmp file by passing -decode as an argument followed by bitmap name." << std::endl;
	}
	else if (ac == 2) {
		if (av[1][0] == '-') {
			std::cout << av[1] << " requires a second argument!" << std::endl;
		}
		else {
			std::cout << "\n** parsing config file" << std::endl;
			auto config = load_config(av[1]);
			if (config.bucket_count() == 0)
				return ABORT("config is empty, aborting operation!");

			std::cout << "\n** attempting to read bitmap: " << config["bitmap"].c_str() << std::endl;
			std::vector<char> bmp_bytes;
			if (!ReadBytes(config["bitmap"].c_str(), bmp_bytes))
				return ABORT("cannot open bitmap.");
			std::cout << "bitmap size: " << bmp_bytes.size() << " bytes" << std::endl;

			std::cout << "\n** attempting to read data: " << config["data"].c_str() << std::endl;
			std::vector<char> data_bytes;
			if (!ReadBytes(config["data"].c_str(), data_bytes))
				return ABORT();
			std::cout << "data size: " << data_bytes.size() << " bytes" << std::endl;

			std::cout << "\n** Encoding started!" << std::endl;
			if (!Encode(bmp_bytes, data_bytes, config)) {
				return ABORT("failed to encode! aborting operation!");
			}

			std::cout << "\n** Creating output file" << std::endl;
			if (!SaveFile(config["output"], bmp_bytes)) {
				return ABORT("failed to save file!");
			}
			std::cout << "succesfully saved output file as " << config["output"].c_str() << std::endl;
		}
	}
	else if (ac == 3) {
		if ((std::string)av[1] == "-new") {
			std::ofstream ofs(av[2], std::ofstream::out);
			if (!ofs.is_open()) {
				std::cout << "error opening new config file" << std::endl;
				return 0;
			}

			ofs << "bitmap:example.bmp" << std::endl;
			ofs << "data:example.example" << std::endl;
			ofs << "output:example.bmp" << std::endl;
			ofs << "bit-offset:0" << std::endl;
			ofs << "encrypt:1" << std::endl;
			ofs << "password_char:0";

			ofs.close();

			std::cout << "opening file..." << std::endl;

			system(av[2]);

			return true;
		}
		else if ((std::string)av[1] == "-decode") {
			std::vector<char> data;
			std::cout << "\n** attempting to read bitmap..." << std::endl;
			if (!ReadBytes(av[2], data)) {
				return ABORT("error reading bitmap! aborting operation!");
			}

			std::vector<char> decoded;
			std::unordered_map<std::string, std::string> config;

			char password_char = '0';
			std::cout << "please enter the password character used to encrypt the data (enter 0 if not used): ";
			std::cin >> password_char;
			std::cout << std::endl;
			config["password_char"] = password_char;

			std::cout << "\n** attempting to extract hidden information..." << std::endl;
			if (!Decode(data, decoded, config)) {
				return ABORT("failed to decode data from bitmap! aborting operation!");
			}
			std::cout << "success!" << std::endl;

			std::cout << "\n** Saving extracted file: \"" << config["data"] << "\" !" << std::endl;
			std::string new_name;
			{
				std::size_t dot_pos = config["data"].find('.');
				new_name += config["data"].substr(0, dot_pos) + " - extracted" + config["data"].substr(dot_pos);
			}
			if (!SaveFile(new_name, decoded)) {
				return ABORT("failed to save file!");
			}
			std::cout << "succesfully saved file as \"" << new_name << "\" !" << std::endl;
		}
	}
	else {
		std::cout << "Too many arguments!" << std::endl;
	}

	std::cout << "\n** please press enter to finish!";
	std::cin.get();

	return 0;
}