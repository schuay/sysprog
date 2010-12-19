int svd_setup(int id, int major);
void svd_remove_dev(int id);
int svd_create(int id, uid_t uid, size_t size, char *key);
int svd_truncate(int id, uid_t uid);
int svd_remove(int id, uid_t uid);
