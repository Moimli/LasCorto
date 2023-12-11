#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <corto/encoder.h>
#include <corto/decoder.h>
#include <laszip/laszip_api.h>

int main(int ac, char **av)
{
    if (ac < 2)
        return -1;

    laszip_POINTER laszip_reader;
    if (laszip_create(&laszip_reader))
    {
        fprintf(stderr, "DLL ERROR: creating laszip reader\n");
        return -1;
    }

    laszip_POINTER laszip_writer;
    if (laszip_create(&laszip_writer))
    {
        fprintf(stderr, "DLL ERROR: creating laszip writer\n");
        return -1;
    }

    laszip_BOOL is_compressed = 0;
    std::ifstream lasFile(av[1], std::ifstream::binary);
    laszip_open_reader_stream(laszip_reader, lasFile, &is_compressed);
    laszip_header *header_read;

    if (laszip_get_header_pointer(laszip_reader, &header_read))
    {
        fprintf(stderr, "DLL ERROR: getting header pointer from laszip reader\n");
        return -1;
    }

    laszip_header *header_write;

    auto start_laz = std::chrono::high_resolution_clock::now();
    if (laszip_get_header_pointer(laszip_writer, &header_write))
    {
        fprintf(stderr, "DLL ERROR: getting header pointer from laszip writer\n");
        return -1;
    }

    laszip_I64 npoints = (header_read->number_of_point_records ? header_read->number_of_point_records : header_read->extended_number_of_point_records);
 
    std::cout << "lasread npoints " << npoints << std::endl;

    header_write->file_source_ID = header_read->file_source_ID;
    header_write->global_encoding = header_read->global_encoding;
    header_write->project_ID_GUID_data_1 = header_read->project_ID_GUID_data_1;
    header_write->project_ID_GUID_data_2 = header_read->project_ID_GUID_data_2;
    header_write->project_ID_GUID_data_3 = header_read->project_ID_GUID_data_3;
    memcpy(header_write->project_ID_GUID_data_4, header_read->project_ID_GUID_data_4, 8);
    header_write->version_major = header_read->version_major;
    header_write->version_minor = header_read->version_minor;
    memcpy(header_write->system_identifier, header_read->system_identifier, 32);
    memcpy(header_write->generating_software, header_read->generating_software, 32);
    header_write->file_creation_day = header_read->file_creation_day;
    header_write->file_creation_year = header_read->file_creation_year;
    header_write->header_size = header_read->header_size;
    header_write->offset_to_point_data = header_read->header_size; /* note !!! */
    header_write->number_of_variable_length_records = 0;
    header_write->point_data_format = header_read->point_data_format;
    header_write->point_data_record_length = header_read->point_data_record_length;
    header_write->number_of_point_records = header_read->number_of_point_records;
    for (int i = 0; i < 5; i++)
    {
        header_write->number_of_points_by_return[i] = header_read->number_of_points_by_return[i];
    }
    header_write->x_scale_factor = header_read->x_scale_factor;
    header_write->y_scale_factor = header_read->y_scale_factor;
    header_write->z_scale_factor = header_read->z_scale_factor;
    header_write->x_offset = header_read->x_offset;
    header_write->y_offset = header_read->y_offset;
    header_write->z_offset = header_read->z_offset;
    header_write->max_x = header_read->max_x;
    header_write->min_x = header_read->min_x;
    header_write->max_y = header_read->max_y;
    header_write->min_y = header_read->min_y;
    header_write->max_z = header_read->max_z;
    header_write->min_z = header_read->min_z;

    if (header_read->user_data_in_header_size)
    {
        header_write->header_size -= header_read->user_data_in_header_size;
        header_write->offset_to_point_data -= header_read->user_data_in_header_size;
        fprintf(stderr, "omitting %d bytes of user_data_in_header\n", header_read->user_data_after_header_size);
    }

    if (laszip_open_writer(laszip_writer, "base.laz", 1))
    {
        fprintf(stderr, "DLL ERROR: opening laszip writer for '%s'\n", "base.laz");
        return -1;
    }

    std::vector<float> coords(npoints * 3);
    std::vector<uint8_t> rgb(npoints * 4);

    laszip_point *point_read;

    if (laszip_get_point_pointer(laszip_reader, &point_read))
    {
        std::cout << "DLL ERROR: getting point pointer from laszip reader" << std::endl;
        return -1;
    }

    laszip_point *point_write;

    if (laszip_get_point_pointer(laszip_writer, &point_write))
    {
        std::cout << "DLL ERROR: getting point pointer from laszip writer" << std::endl;
        return -1;
    }

    laszip_I64 p_count = 0;

    while (p_count < npoints)
    {
        // read a point

        if (laszip_read_point(laszip_reader))
        {
            std::cout << "DLL ERROR: reading point " << p_count << std::endl;
            return -1;
        }

        point_write->X = point_read->X;
        point_write->Y = point_read->Y;
        point_write->Z = point_read->Z;
        memcpy(point_write->rgb, point_read->rgb, 8);

        if (laszip_write_point(laszip_writer))
        {
            fprintf(stderr, "DLL ERROR: writing point %I64d\n", p_count);
            return -1;
        }

        coords[p_count * 3] = (float)point_read->X;
        coords[p_count * 3 + 1] = (float)point_read->Y;
        coords[p_count * 3 + 2] = (float)point_read->Z;
        for (size_t i = 0; i < 4; i++)
        {
            rgb[p_count * 4 + i] = (uint8_t)(point_read->rgb[i] >> 8);
        }

        p_count++;
    }

    auto end_laz = std::chrono::high_resolution_clock::now();
    std::cout << "laz time " << std::chrono::duration_cast< std::chrono::milliseconds >(end_laz - start_laz) << std::endl;
    auto start_corto = std::chrono::high_resolution_clock::now();
    std::ofstream lasCortoFile("output.lascorto");
    crt::Encoder encoder(npoints, 0);

    // add attributes to be encoded
    encoder.addPositionsBits(coords.data(), 9);
    encoder.addColors(rgb.data(), 2, 2, 2, 0);
    std::cout << "ready for encoding" << std::endl;
    encoder.encode();
    lasCortoFile.write((char*)encoder.stream.data(), encoder.stream.size());
    auto end_corto = std::chrono::high_resolution_clock::now();
    std::cout << "corto size : " << encoder.stream.size() << " time : " << std::chrono::duration_cast< std::chrono::milliseconds >(end_corto - start_corto) << std::endl;

    crt::Decoder decoder(encoder.stream.size(), encoder.stream.data());

    unsigned int nvert = decoder.nvert;
    std::vector<float> icoords(nvert * 3);
    std::vector<uint8_t> icolors(nvert * 4);

    decoder.setPositions(icoords.data());
    std::cout << "Decoder setPositions" << std::endl;
    decoder.setColors(icolors.data());
    std::cout << "Decoder setColors" << std::endl;
    decoder.decode();
    std::cout << "Decoded" << std::endl;
    laszip_POINTER laszipcorto_writer;
    if (laszip_create(&laszipcorto_writer))
    {
        std::cout << "DLL ERROR: creating laszip writer" << std::endl;
        return -1;
    }

    laszip_header *headercorto_write;

    if (laszip_get_header_pointer(laszipcorto_writer, &headercorto_write))
    {
        std::cout << "DLL ERROR: getting header pointer from laszip corto writer" << std::endl;
        return -1;
    }

    headercorto_write->file_source_ID = header_read->file_source_ID;
    headercorto_write->global_encoding = header_read->global_encoding;
    headercorto_write->project_ID_GUID_data_1 = header_read->project_ID_GUID_data_1;
    headercorto_write->project_ID_GUID_data_2 = header_read->project_ID_GUID_data_2;
    headercorto_write->project_ID_GUID_data_3 = header_read->project_ID_GUID_data_3;
    memcpy(headercorto_write->project_ID_GUID_data_4, header_read->project_ID_GUID_data_4, 8);
    headercorto_write->version_major = header_read->version_major;
    headercorto_write->version_minor = header_read->version_minor;
    memcpy(headercorto_write->system_identifier, header_read->system_identifier, 32);
    memcpy(headercorto_write->generating_software, header_read->generating_software, 32);
    headercorto_write->file_creation_day = header_read->file_creation_day;
    headercorto_write->file_creation_year = header_read->file_creation_year;
    headercorto_write->header_size = header_read->header_size;
    headercorto_write->offset_to_point_data = header_read->header_size; /* note !!! */
    headercorto_write->number_of_variable_length_records = 0;
    headercorto_write->point_data_format = header_read->point_data_format;
    headercorto_write->point_data_record_length = header_read->point_data_record_length;
    headercorto_write->number_of_point_records = header_read->number_of_point_records;
    for (int i = 0; i < 5; i++)
    {
        headercorto_write->number_of_points_by_return[i] = header_read->number_of_points_by_return[i];
    }
    headercorto_write->x_scale_factor = header_read->x_scale_factor;
    headercorto_write->y_scale_factor = header_read->y_scale_factor;
    headercorto_write->z_scale_factor = header_read->z_scale_factor;
    headercorto_write->x_offset = header_read->x_offset;
    headercorto_write->y_offset = header_read->y_offset;
    headercorto_write->z_offset = header_read->z_offset;
    headercorto_write->max_x = header_read->max_x;
    headercorto_write->min_x = header_read->min_x;
    headercorto_write->max_y = header_read->max_y;
    headercorto_write->min_y = header_read->min_y;
    headercorto_write->max_z = header_read->max_z;
    headercorto_write->min_z = header_read->min_z;

    if (header_read->user_data_in_header_size)
    {
        headercorto_write->header_size -= header_read->user_data_in_header_size;
        headercorto_write->offset_to_point_data -= header_read->user_data_in_header_size;
        std::cout << "omitting" << header_read->user_data_after_header_size << "bytes of user_data_in_header" << std::endl;
    }

    if (laszip_open_writer(laszipcorto_writer, "corto.laz", 1))
    {
        std::cout << "DLL ERROR: opening laszip writer for 'corto.laz'" << std::endl;
        return -1;
    }

    laszip_point *pointcorto_write;

    if (laszip_get_point_pointer(laszipcorto_writer, &pointcorto_write))
    {
        std::cout << "DLL ERROR: getting point pointer from laszip writer" << std::endl;
        return -1;
    }

    p_count = 0;

    while (p_count < nvert)
    {
        pointcorto_write->X = icoords[p_count * 3];
        pointcorto_write->Y = icoords[p_count * 3 + 1];
        pointcorto_write->Z = icoords[p_count * 3 + 2];
        for (size_t i = 0; i < 5; i++)
        {
            pointcorto_write->rgb[i] = ((uint16_t)icolors[p_count * 4 + i]) << 8;
        }
        

        if (laszip_write_point(laszipcorto_writer))
        {
            std::cout << "DLL ERROR: writing point " << p_count << std::endl;
            return -1;
        }

        p_count++;
    }

    std::cout << "finished" << std::endl;
    return 0;
}