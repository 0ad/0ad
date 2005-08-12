void* ogg_create();

void ogg_give_raw(void* o, void* p, size_t size);

void ogg_open(void* o, ALenum& fmt, ALsizei& freq);

size_t ogg_read(void* o, void* buf, size_t max_size);

void ogg_release(void* o);
