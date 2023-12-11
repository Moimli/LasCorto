#include <iostream>
#include <iomanip>
#include <fstream>
#include <corto/encoder.h>
#include <corto/decoder.h>
#include <laszip/laszip_api.h>

int main(int ac, char **av)
{
    crt::Encoder encoder(20);
    std::vector<float> ocoords(60);
    for (size_t i = 0; i < 60; i++)
    {
        ocoords[i] = i;
    }
    encoder.addPositions(ocoords.data());
    encoder.encode();

    std::ofstream lasCortoFileOut("output.lascortoT");
    lasCortoFileOut.write((char *)encoder.stream.data(), encoder.stream.size());
    lasCortoFileOut.close();
    std::cout << "test file writen" << std::endl;
    std::ifstream lasCortoFileIn("output.lascortoT");
    lasCortoFileIn.seekg(0, lasCortoFileIn.end);
    int length = lasCortoFileIn.tellg();
    lasCortoFileIn.seekg(0, lasCortoFileIn.beg);

    auto buffer = std::make_unique<char[]>(length);
    lasCortoFileIn.read(buffer.get(), length);

    crt::Decoder decoder(length, (unsigned char *)buffer.get());
    std::cout << "Created decoder for output.lascorto of length " << length << std::endl;

    unsigned int nvert = decoder.nvert;

    std::cout << "Decoder nverts " << nvert << std::endl;
    std::vector<float> icoords(nvert * 3);
    std::vector<laszip_U16> icolors(nvert * 4);

    decoder.setPositions(icoords.data());
    std::cout << "Decoder setPositions" << std::endl;
    // decoder.setAttribute("rgb", (char *)icolors.data(), crt::VertexAttribute::INT16);
    // std::cout << "Decoder setAttribute rgb" << std::endl;
    decoder.decode();
    std::cout << "Decoded output.lascorto" << std::endl;

    for (size_t i = 0; i < nvert; i++)
    {
        std::cout << icoords[i * 3] << " " << icoords[i * 3 + 1] << " " << icoords[i * 3 + 2] << std::endl;
    }

    std::cout << "finished" << std::endl;
    return 0;
}