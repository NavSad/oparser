//
//  main.cpp
//  oparser
//
//  Created by NavSad on 9/28/20.
//

#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <mach-o/loader.h>
#include <mach-o/swap.h>

using namespace std;

mach_header_64 *header;
fat_header *fat_hdr;
fat_arch *fat_ar=NULL;

bool is_mach_o_file(int fd,struct stat *file_stat,void **file);

void search_for_string(int fd,struct stat *file_stat,void *file,string &text_name,bool is_output,string &output_file);

void cleanup(int fd, void *file);

int main(int argc, const char * argv[])
{
    struct stat *file_stat=new(struct stat);
    void *file=NULL;
    if(argc<2)
    {
        cerr<<"Too few arguments."<<"\n";
        return 1;
    }
    if(strncmp(argv[1], "-h", 2)==0 || strncmp(argv[1], "help", 4)==0)
    {
        cout<<"./oparser <options> <filename>"<<"\n";
        cout<<"-f <textname> (Searches for the string in the text region.)"<<"\n";
        cout<<"-t (Parses the text region.)"<<"\n";
        cout<<"-d (Parses the data region.)"<<"\n";
        cout<<"-a (Parses the whole file.)"<<"\n";
        cout<<"-o <filename> (Outputs the parses data to a file. Can be used with any option. Used after the file you want to parse.)"<<"\n";
    }
    if(strncmp(argv[1], "-f", 2)==0)
    {
        //cout<<"Made it here."<<"\n";
        string text_name=argv[2];
        string file_name=argv[3];
        string output_file;
        bool is_output=false;
       /* if(strncmp(argv[4], "-o", 2)==0)
        {
            output_file=argv[5];
        }*/
        cout<<text_name<<"\n";
        cout<<file_name<<"\n";
        int fd=open(file_name.c_str(),O_RDONLY);
        if(fd<0)
        {
            cerr<<"Failed to open file."<<"\n";
            return 1;
        }
        //cout<<"Made it here."<<"\n";
        bool is_mach_file=is_mach_o_file(fd, file_stat, &file);
        //cout<<"Made it here."<<"\n";
        if(is_mach_file==false)
        {
            cerr<<"Not a Mach-O file."<<"\n";
            int ret=munmap(file,file_stat->st_size);
            if(ret<0)
            {
                cerr<<"Failed to unmap file."<<"\n";
                close(fd);
                return 1;
            }
            ret=close(fd);
            if(ret<0)
            {
                cerr<<"Failed to close file."<<"\n";
                return 1;
            }
            return 1;
        }
        //cout<<"Made it here."<<"\n";
        search_for_string(fd, file_stat, file, text_name, is_output, output_file);
    }
    return 0;
}

bool is_mach_o_file(int fd,struct stat *file_stat,void **file)
{
    header=new(mach_header_64);
    int ret=fstat(fd,file_stat);
    if(ret<0)
    {
        cerr<<"Failed to get file size."<<"\n";
        exit(1);
    }
    *file=mmap(NULL, file_stat->st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //cout<<"Made it here."<<"\n";
    if(*file==MAP_FAILED)
    {
        cerr<<"Failed to map file."<<"\n";
        exit(1);
    }
    uint32_t *file_copy=(uint32_t*)(*file);
    if(*file_copy==MH_MAGIC_64 || *file_copy==MH_CIGAM_64)
    {
        memcpy(header,*file,sizeof(mach_header_64));
        return true;
    }
    if(*file_copy==MH_MAGIC || *file_copy==MH_CIGAM)
    {
        cerr<<"32-bit Mach-O files are not supported at the moment."<<"\n";
        ret=munmap(*file, file_stat->st_size);
        if(ret<0)
        {
            cerr<<"Failed to unmap file."<<"\n";
            exit(1);
        }
        ret=close(fd);
        if(ret<0)
        {
            cerr<<"Failed to close file."<<"\n";
            exit(1);
        }
        exit(1);
    }
    if(*file_copy==FAT_MAGIC_64 || *file_copy==FAT_CIGAM_64 || *file_copy==FAT_MAGIC || *file_copy==FAT_CIGAM)
    {
        memcpy(fat_hdr,*file,sizeof(fat_header));
        //Do something for fat_arch_64
        fat_ar=new fat_arch[fat_hdr->nfat_arch];
        fat_arch *fat_ar_copy=(fat_arch*)(*file);
        fat_ar_copy=fat_ar_copy+sizeof(fat_header);
        for(int i=0;i<fat_hdr->nfat_arch;i++)
        {
            fat_ar[i]=*(fat_ar_copy);
            fat_ar_copy++;
        }
    }
    //uint32_t *file_copy=(uint32_t*)file;
    /*if(*file_copy!=MH_MAGIC_64 && *file_copy!=MH_CIGAM_64)
    {
        return false;
    }*/
    /*
    header->magic=*(file_copy);
    file_copy++;
    integer_t *file_copy_i=(integer_t*)file_copy;
    header->cputype=*(file_copy_i);
    file_copy_i++;
    header->cpusubtype=*(file_copy_i);
    file_copy_i++;
    file_copy=(uint32_t*)file_copy_i;
    header->filetype=*(file_copy);
    file_copy++;
    header->ncmds=*(file_copy);
    file_copy++;
    header->sizeofcmds=*(file_copy);
    file_copy++;
    header->flags=*(file_copy);
    file_copy++;
    header->reserved=*(file_copy);
     */
    return false;
}

void search_for_string(int fd,struct stat *file_stat,void *file,string &text_name,bool is_output,string &output_file)
{
    //cout<<"Made it here."<<"\n";
    if(fat_ar!=NULL)
    {
        
    }
    segment_command_64 *seg_com_64;
    segment_command *seg_com;
    load_command *lc;
    section_64 *sec_64;
    section *sec;
    char buf[text_name.length()];
    char *buf_ptr=buf;
    uint32_t *file_copy=(uint32_t*)file;
    file_copy=file_copy+sizeof(mach_header_64);
    cout<<"Made it here."<<"\n";
    for(int i=0;i<header->ncmds;i++)
    {
        //cout<<"Made it here."<<"\n";
        lc=new load_command;
        //cout<<*file_copy<<"\n";
        memcpy(lc,file_copy,sizeof(load_command));
        //cout<<"Made it here."<<"\n";
        if(lc->cmd==LC_SEGMENT)
        {
            //cout<<"Made it here."<<"\n";
            seg_com=new(segment_command);
            memcpy(seg_com, file_copy, sizeof(segment_command));
            if(strncmp(seg_com->segname, "__TEXT", 6)==0)
            {
                file_copy=file_copy+sizeof(segment_command);
                sec=new section[seg_com->nsects];
                memcpy(sec, file_copy, seg_com->nsects*sizeof(section));
                for(int j=0;j<seg_com->nsects;j++)
                {
                    if(strncmp(sec[j].sectname, "__TEXT.__cstring", 16)==0)
                    {
                        file_copy=(uint32_t*)file+sec[j].offset;
                        char *file_copy_char=(char*)file_copy;
                        int counter=0;
                        for(int k=0;k<sec[j].size;k++)
                        {
                            buf[counter]=*(file_copy_char);
                            if(counter-1==text_name.length())
                            {
                                if(strncmp(buf_ptr,text_name.c_str(), text_name.length())==0)
                                {
                                    cout<<"Text '"<<text_name<<"' found at address "<<file_copy_char-text_name.length()<<"."<<"\n";
                                    if(is_output==true)
                                    {
                                        goto output;
                                    }
                                    if(is_output==false)
                                    {
                                        delete []sec;
                                        cleanup(fd, file);
                                    }
                                    }
                                    else
                                    {
                                        counter=0;
                                        continue;
                                    }
                                }
                                file_copy_char++;
                                counter++;
                            }
                        }
                        file_copy=file_copy+sizeof(section_64);
                    }
                }
                file_copy=file_copy+seg_com->cmdsize;
                delete seg_com;
                delete lc;
                continue;
        }
        if(lc->cmd==LC_SEGMENT_64)
        {
            //cout<<"Made it here."<<"\n";
            seg_com_64=new(segment_command_64);
            memcpy(seg_com_64, file_copy, sizeof(segment_command_64));
            if(strncmp(seg_com_64->segname, "__TEXT", 6)==0)
            {
                file_copy=file_copy+sizeof(segment_command_64);
                sec_64=new section_64[seg_com_64->nsects];
                memcpy(sec_64, file_copy, seg_com_64->nsects*sizeof(section_64));
                for(int j=0;j<seg_com_64->nsects;j++)
                {
                    if(strncmp(sec_64[j].sectname, "__TEXT.__cstring", 16)==0)
                    {
                        file_copy=(uint32_t*)file+sec_64[j].offset;
                        char *file_copy_char=(char*)file_copy;
                        int counter=0;
                        for(int k=0;k<sec_64[j].size;k++)
                        {
                            buf[counter]=*(file_copy_char);
                            if(counter-1==text_name.length())
                            {
                                if(strncmp(buf_ptr,text_name.c_str(), text_name.length())==0)
                                {
                                    cout<<"Text '"<<text_name<<"' found at address "<<file_copy_char-text_name.length()<<"."<<"\n";
                                    if(is_output==true)
                                    {
                                        goto output;
                                    }
                                    if(is_output==false)
                                    {
                                        delete []sec_64;
                                        cleanup(fd, file);
                                    }
                                }
                                else
                                {
                                    counter=0;
                                    continue;
                                }
                            }
                            file_copy_char++;
                            counter++;
                        }
                    }
                    file_copy=file_copy+sizeof(section_64);
                }
            }
            file_copy=file_copy+seg_com_64->cmdsize;
            delete seg_com_64;
            delete lc;
            continue;
        }
        file_copy=file_copy+lc->cmdsize;
        delete lc;
    }
    output:if(is_output==true)
    {
        
    }
}

void cleanup(int fd,void *file)
{
    
}

