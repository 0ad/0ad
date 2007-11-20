#include "codec.h"

extern boost::shared_ptr<ICodec> CreateCodec_ZLibNone();
extern boost::shared_ptr<ICodec> CreateCompressor_ZLibDeflate();
extern boost::shared_ptr<ICodec> CreateDecompressor_ZLibDeflate();
