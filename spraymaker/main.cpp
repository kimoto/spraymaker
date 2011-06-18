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
  int size = ::WideCharToMultiByte(CP_UTF8,0,utf16_string,wcslen(utf16_string)+1,NULL,0,NULL,NULL);
  char *output = new char[size];
  int writed_size = ::WideCharToMultiByte(CP_UTF8,0,utf16_string,wcslen(utf16_string)+1,output,size,NULL,NULL);

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

// 指定されたファイルを、指定されたファイル名のvtfに保存します
int convertToHighQuaritySprayVTF(int size, LPCTSTR inputPath, LPCTSTR outputPath, VTFImageFormat type = IMAGE_FORMAT_RGBA8888)
{
  if(!::PathFileExists(inputPath)){
    return -1;
  }
  
  // TODO: 入力ファイルの拡張子がvtfであればCVTFFileで読み込んでimagemagickに転送してあげる
  // read by ImageMagick
  ::Magick::Image image;
  image.type(::Magick::TrueColorType);
  image.modifyImage();
  char *utf8_inputPath = ::utf16_to_utf8(inputPath);
  image.read(utf8_inputPath);
  delete [] utf8_inputPath;

  image.resize(Magick::Geometry(size, size));
  ::msgbox(L"resized: w:%d, h:%d", image.size().width(), image.size().height());

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
  for(int x=0; x<size; x++){
    for(int y=0; y<size; y++){
      int i = (y * size + x) * 4;

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

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
  ::convertToHighQuaritySprayVTF(256, L"test.vtf", L"test2.vtf");
  return 0;
  for(int i=1; i<__argc; i++){
    LPCTSTR input_path = __targv[i];

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
        continue; // 次の要素 or 終了処理へ
      }
    }

    if( ::convertToHighQuaritySprayVTF(256, input_path, output_path, ::IMAGE_FORMAT_BGRA8888) != 0 ){
      ::msgbox(L"ファイル書き込みに失敗しました: %s", output_path);
    }
    delete [] output_path;
  }
  return 0;
}
