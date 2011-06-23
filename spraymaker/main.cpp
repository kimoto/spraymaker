/*
  spraymaker
  description is here: http://github.com/kimoto/spraymaker/

  not working ImageMagick on DEBUG mode :(
  please compile RELEASE mode :p

  author: kimoto;
 */
#pragma once

#include <Windows.h>
#include <VTFLib.h>
#pragma comment(lib, "VTFLib.lib")
using namespace VTFLib;

#pragma warning(push)
#pragma warning(disable : 4251)
#include <Magick++.h>
#pragma warning(pop)
#pragma comment(lib, "CORE_RL_Magick++_.lib")

#include <string>
using namespace std;

#include "ut.h"

// dynamic allocate character encode function
//  @param code_page: code page (example: CP_UTF8, CP_THREAD_ACP)
//  @param utf16_string: utf16_string buffer pointer
//  @return output: encoded / allocated buffer pointer
//
//  note:
//    you need to delete returned buffer;
char *utf16_to(UINT code_page, LPCTSTR utf16_string)
{
  int size = ::WideCharToMultiByte(code_page,0,utf16_string,wcslen(utf16_string)+1,NULL,0,NULL,NULL);
  char *output = new char[size];
  int writed_size = ::WideCharToMultiByte(code_page,0,utf16_string,wcslen(utf16_string)+1,output,size,NULL,NULL);

  if(size == writed_size)
    return output;
  return NULL;
}

// UTF-16(wide char) to UTF-8
char *utf16_to_utf8(LPCTSTR utf16_string)
{
  return ::utf16_to(CP_UTF8, utf16_string);
}

// UTF-16 to multibyte character string
char *utf16_to_sjis(LPCTSTR utf16_string)
{
  return ::utf16_to(CP_THREAD_ACP, utf16_string);
}

// write CVTFfile class to filesystem
//  @param writePath: output path
//  @param *file: CVTFfile pointer
void write_to_file_CVTFFile(LPCTSTR writePath, CVTFFile *file)
{
  // allocate buffer for VTF file size
  vlUInt vtf_size = file->GetSize();
  vlByte *buffer = new vlByte[vtf_size];
  vlUInt writed = 0;
  file->Save(buffer, vtf_size, writed); // output to memory buffer

  // output to filesystem. use UTF-16 wide chars
  FILE *fp = NULL;
  ::_wfopen_s(&fp, writePath, L"wb");
  fwrite(buffer, sizeof(vlByte), writed, fp);
  fclose(fp);

  delete [] buffer;
}

// case insentive widechars compare
//  @return true: match (case insentive)
//          false: not match
bool wcscmpi(LPCTSTR str1, LPCTSTR str2)
{
  if( ::wcslen(str1) != ::wcslen(str2) ){
    return false;
  }

  for(unsigned int i=0; i<::wcslen(str1); i++){
    if( ::towlower(str1[i]) != ::towlower(str2[i]) ){
      return false;
    }
  }
  return true;
}

// Is VTF file name?
//  @return true: yes. VTF file
//          false: no
bool is_vtf_file(LPCTSTR path)
{
  TCHAR drive[MAX_PATH];
  TCHAR dir[MAX_PATH];
  TCHAR filename[MAX_PATH];
  TCHAR ext[MAX_PATH];
  ::_wsplitpath_s(path, drive, dir, filename, ext);
  return ::wcscmpi(ext, L".vtf");
}

//
// Convert image file inputPath to outputPath
// auto recognize filetype by file extension. (example vtf => VTF file, jpg => JPEG file)
//
//  @param
//          size: spray image size, example if 256 taken then 256x256 spray create.
//          inputPath: input file path (need to exist)
//          outputPath: output file path
//          type: VTFImageFormat (default is IMAGE_FORMAT_RGBA8888)
//  @return
//          -1: any error!
//          0: successfull!
int convertToHighQuaritySprayVTF(unsigned int size, LPCTSTR inputPath, LPCTSTR outputPath, VTFImageFormat type = IMAGE_FORMAT_RGBA8888)
{
  if(!::PathFileExists(inputPath)){
    return -1;
  }

  ::Magick::Image image;
  image.type(::Magick::TrueColorType);
  image.modifyImage();

  if( is_vtf_file(inputPath) ){
    // load image by VTFLib
    char *sjis_path = ::utf16_to_sjis(inputPath);

    CVTFFile file;
    file.GetDepth();
    file.Load(sjis_path);

    // memory copy vtflib's buffer to image magick's buffer
    CVTFFile tmpfile;
    tmpfile.Create(file.GetWidth(), file.GetHeight());
    file.ConvertToRGBA8888(file.GetData(1,1,1,0), tmpfile.GetData(1,1,1,0), file.GetWidth(), file.GetHeight(), file.GetFormat());
    
    image.read(tmpfile.GetWidth(), tmpfile.GetHeight(), "RGBA", Magick::CharPixel, tmpfile.GetData(1,1,1,0));

    delete [] sjis_path;
  }else{
    try{
      char *utf8_inputPath = ::utf16_to_utf8(inputPath);
      image.read(utf8_inputPath);
      delete [] utf8_inputPath;
    }catch(Magick::Exception e){
      // if not read image file, no operation
    }
  }

  // resize specified image size (size x size)
  image.resize(Magick::Geometry(size, size));

  // create output image file structure
  CVTFFile file;
  file.Create(size, size);
  vlByte *byte = file.GetData(1, 1, 1, 0);
  if(byte == NULL){
    return -1;
  }

  // imagemagick's resized buffer to output VTFlib file structure
  int w = image.size().width();
  int h = image.size().height();
  image.write(0, 0, size, size, "RGBA", Magick::CharPixel, byte);

  // padding transparent pixels
  for(unsigned int x=0; x<size; x++){
    for(unsigned int y=0; y<size; y++){
      unsigned int i = (y * size + x) * 4;

      if(image.size().width() < x || image.size().height() < y){
        byte[i+0] = byte[i+1] = byte[i+2] = byte[i+3] = 0;
      }
    }
  }

  // CVTFFile::Save is not handled Unicode(wide character) file name(omg)
  // so not use CVTFFile::Save, substitute use _wfopen_s & fwrite windows sdk functions.
  CVTFFile convert_file;
  convert_file.Create(size, size, 1, 1, 1, type);

  file.ConvertFromRGBA8888(file.GetData(1,1,1,0), convert_file.GetData(1,1,1,0), size, size, type);

  convert_file.SetFlag(::TEXTUREFLAGS_ANISOTROPIC, true);
  convert_file.SetFlag(TEXTUREFLAGS_NOMIP, true);
  convert_file.SetFlag(TEXTUREFLAGS_NOLOD, true);
  write_to_file_CVTFFile(outputPath, &convert_file);
  return 0;
}

void convertToHighQuaritySprayVTF_Wrap(LPCTSTR input_path)
{
  TCHAR drive[MAX_PATH];
  TCHAR dir[MAX_PATH];
  TCHAR filename[MAX_PATH];
  TCHAR ext[MAX_PATH];
  ::_wsplitpath_s(input_path, drive, dir, filename, ext);
  LPCTSTR output_path = ::wsprintf_alloc(L"%s%s%s.vtf", drive, dir, filename);

  if( ::PathFileExists(output_path) ){
    UINT ret = msgbox(L"Confirm", MB_YESNO, L"already exist in the directory. overwrite?\r\n%s", output_path);
    if(ret != IDYES){
      return; // do no convert
    }
  }

  // convert to vtf file (256x256) and BGRA8888
  if( ::convertToHighQuaritySprayVTF(256, input_path, output_path, ::IMAGE_FORMAT_BGRA8888) != 0 ){
    ::msgbox(L"cannot write spray: %s", output_path);
  }else{
    ::MessageBeep(MB_ICONASTERISK);
  }
  delete [] output_path;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPTSTR    lpCmdLine,
  int       nCmdShow)
{
  if(__argc <= 1){
    // if empty arguments
  }else{
    for(int i=1; i<__argc; i++){
      LPCTSTR input_path = __targv[i];
      if(::PathFileExists(input_path)){
        if(::PathIsDirectory(input_path)){
          each_directory(input_path, convertToHighQuaritySprayVTF_Wrap, true);
        }else{
          convertToHighQuaritySprayVTF_Wrap(input_path);
        }
      }
    }
  }
  return 0;
}
