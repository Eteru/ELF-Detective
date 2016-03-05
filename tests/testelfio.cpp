#include <iostream>
#include <cassert>
#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>

using namespace ELFIO;

int main (int argc, char** argv)
{
	if (argc < 2) {
		std::cout << "Usage: testapp <elf_file>" << std::endl;
	}

	elfio reader;

	// Load ELF file
	assert(reader.load(argv[1]) && "Bad ELF file");

	// Print ELF file properties
	std::cout << "ELF file class : ";
	if ( reader.get_class() == ELFCLASS32 )
	 std::cout << "ELF32" << std::endl;
	else
	 std::cout << "ELF64" << std::endl;

	std::cout << "ELF file encoding : ";
	if ( reader.get_encoding() == ELFDATA2LSB )
	 std::cout << "Little endian" << std::endl;
	else
	 std::cout << "Big endian" << std::endl;

	Elf_Half sec_num = reader.sections.size();

	std::cout << "Number of sections: " << sec_num << std::endl;

	dump::header  ( std::cout, reader );
	dump::section_datas  ( std::cout, reader );

	return 0;
}