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

char *utf16_to(UINT code_page, LPCTSTR utf16_string)
{
  // ccMultibyteに0を指定すると変換後のバッファに必要なバイト数が入る
  int size = ::WideCharToMultiByte(code_page,0,utf16_string,wcslen(utf16_string)+1,NULL,0,NULL,NULL);
  char *output = new char[size];
  int writed_size = ::WideCharToMultiByte(code_page,0,utf16_string,wcslen(utf16_string)+1,output,size,NULL,NULL);

  if(size == writed_size)
    return output;
  return NULL;
}

char *utf16_to_utf8(LPCTSTR utf16_string)
{
  return ::utf16_to(CP_UTF8, utf16_string);
}

char *utf16_to_sjis(LPCTSTR utf16_string)
{
  return ::utf16_to(CP_THREAD_ACP, utf16_string);
}

void write_to_file_CVTFFile(LPCTSTR writePath, CVTFFile *file)
{
  // VTFのファイルサイズ分だけ出力用のバッファを確保します
  vlUInt vtf_size = file->GetSize();
  vlByte *buffer = new vlByte[vtf_size];
  vlUInt writed = 0;
  file->Save(buffer, vtf_size, writed); // メモリ上に出力

  // utf16ファイル名でファイルに出力します
  FILE *fp = NULL;
  ::_wfopen_s(&fp, writePath, L"wb");
  fwrite(buffer, sizeof(vlByte), writed, fp);
  fclose(fp);

  delete [] buffer;
}

// wcscmpと違うのは、二つの文字が大文字区別して一致するかどうか
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

bool is_vtf_file(LPCTSTR path)
{
  TCHAR drive[MAX_PATH];
  TCHAR dir[MAX_PATH];
  TCHAR filename[MAX_PATH];
  TCHAR ext[MAX_PATH];
  ::_wsplitpath_s(path, drive, dir, filename, ext);
  return ::wcscmpi(ext, L".vtf");
}

// 指定されたファイルを、指定されたファイル名のvtfに保存します
int convertToHighQuaritySprayVTF(unsigned int size, LPCTSTR inputPath, LPCTSTR outputPath, VTFImageFormat type = IMAGE_FORMAT_RGBA8888)
{
  if(!::PathFileExists(inputPath)){
    return -1;
  }

  ::Magick::Image image;
  image.type(::Magick::TrueColorType);
  image.modifyImage();

  // 入力パスがVTFファイルだったらVTFをデコードしてimagemagickに格納する
  if( is_vtf_file(inputPath) ){
    char *sjis_path = ::utf16_to_sjis(inputPath);

    CVTFFile file;
    file.GetDepth();
    file.Load(sjis_path);

    // 読み込んだら使いやすいフォーマットに変換します(RGBA8888)
    CVTFFile tmpfile;
    tmpfile.Create(file.GetWidth(), file.GetHeight());
    file.ConvertToRGBA8888(file.GetData(1,1,1,0), tmpfile.GetData(1,1,1,0), file.GetWidth(), file.GetHeight(), file.GetFormat());

    // image magick側でRGBAとして読み込みます
    image.read(tmpfile.GetWidth(), tmpfile.GetHeight(), "RGBA", Magick::CharPixel, tmpfile.GetData(1,1,1,0));

    delete [] sjis_path;
  }else{
    try{
      char *utf8_inputPath = ::utf16_to_utf8(inputPath);
      image.read(utf8_inputPath);
      delete [] utf8_inputPath;
    }catch(Magick::Exception e){
      // 読み込めなかったとき、無視します
    }
  }

  image.resize(Magick::Geometry(size, size));
  //::msgbox(L"resized: w:%d, h:%d", image.size().width(), image.size().height());

  // create output image
  CVTFFile file;
  file.Create(size, size);
  vlByte *byte = file.GetData(1, 1, 1, 0);
  if(byte == NULL){
    return -1;
  }

  // Magickのデータを、VTFのメモリ空間に書きこみ
  // 中央寄せします
  int w = image.size().width();
  int h = image.size().height();
  image.write(0, 0, size, size, "RGBA", Magick::CharPixel, byte);

  // 完全に透過な黒で初期化
  // リサイズしたときの余白部分は透明になります
  for(unsigned int x=0; x<size; x++){
    for(unsigned int y=0; y<size; y++){
      unsigned int i = (y * size + x) * 4;

      if(image.size().width() < x || image.size().height() < y){
        byte[i+0] = byte[i+1] = byte[i+2] = byte[i+3] = 0;
      }
    }
  }

  // CVTFFileがunicodeに対応していないので日本語ファイル名が扱えない
  // そこでCVTFFilesにはメモリ上にVTFデータを出力させて
  // それをwindowsのファイル書き込みAPIでUnicodeファイル名に対応させることにした
  // もし圧縮形式が指定されていた場合はその形式に変換します
  CVTFFile convert_file;
  convert_file.Create(size, size, 1, 1, 1, type);

  file.ConvertFromRGBA8888(file.GetData(1,1,1,0), convert_file.GetData(1,1,1,0), size, size, type);

  convert_file.SetFlag(::TEXTUREFLAGS_ANISOTROPIC, true);
  convert_file.SetFlag(TEXTUREFLAGS_NOMIP, true);
  convert_file.SetFlag(TEXTUREFLAGS_NOLOD, true);

  // ファイルに出力
  write_to_file_CVTFFile(outputPath, &convert_file);
  return 0;
}

void test_output_to_file(const char *path, const char *data)
{
  FILE *fp = NULL;
  ::fopen_s(&fp, path, "wb");
  fputs(data, fp);
  ::fclose(fp);
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
    UINT ret = msgbox(L"確認", MB_YESNO, L"出力先ファイルが存在しています、上書きしてもよろしいですか?\r\n%s", output_path);
    if(ret == IDYES){
      ; // データ書き込み処理へ
    }else{
      return; // 次の要素へ
    }
  }

  if( ::convertToHighQuaritySprayVTF(256, input_path, output_path, ::IMAGE_FORMAT_BGRA8888) != 0 ){
    ::msgbox(L"ファイル書き込みに失敗しました: %s", output_path);
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
    // 引数がなかったときはキャプチャモード
    //::msgbox(L"キャプチャモード");
  }else{
    for(int i=1; i<__argc; i++){
      LPCTSTR input_path = __targv[i];
      if(::PathFileExists(input_path)){
        if(::PathIsDirectory(input_path)){ // ディレクトリの時は再帰で
          each_directory(input_path, convertToHighQuaritySprayVTF_Wrap, true);
        }else{
          convertToHighQuaritySprayVTF_Wrap(input_path);
        }
      }
    }
  }
  return 0;
}
