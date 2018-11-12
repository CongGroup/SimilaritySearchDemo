/***********************************************************
 * Vss Thrift Script
 * Made By XinyuWang
 * 2014/2/23
 ***********************************************************/

namespace java ImgMatch
namespace cpp ImgMatch
namespace rb ImgMatch
namespace py ImgMatch
namespace perl ImgMatch
namespace csharp ImgMatch

//"required" and "optional" keywords are purely for documentation.


service TMatchServer {

  /**
   * Get the Binary of the Picture
   * @return the binary
   */
  binary GetImage(
    1: required string fid
  );
  

  /**
   * Get the picture id list which is similar.
   * @return the fid list.
   */
  list<string> SearchSimilar(
    1: required binary img,
    2: required bool add
  );
  
}
