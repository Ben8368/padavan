/*
 * Stub implementations for missing symbols
 */

#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/proc_fs.h>

/* Stub for procRegDir */
struct proc_dir_entry *procRegDir = NULL;
EXPORT_SYMBOL(procRegDir);

/* Stub implementations for MTD functions */
int ra_mtd_read_nm(char *name, loff_t from, size_t len, u_char *buf)
{
	struct mtd_info *mtd;
	size_t retlen;
	int ret;

	mtd = get_mtd_device_nm(name);
	if (IS_ERR(mtd))
		return PTR_ERR(mtd);

	ret = mtd_read(mtd, from, len, &retlen, buf);
	put_mtd_device(mtd);

	return ret;
}
EXPORT_SYMBOL(ra_mtd_read_nm);

int ra_mtd_write_nm(char *name, loff_t to, size_t len, const u_char *buf)
{
	struct mtd_info *mtd;
	size_t retlen;
	int ret;

	mtd = get_mtd_device_nm(name);
	if (IS_ERR(mtd))
		return PTR_ERR(mtd);

	ret = mtd_write(mtd, to, len, &retlen, buf);
	put_mtd_device(mtd);

	return ret;
}
EXPORT_SYMBOL(ra_mtd_write_nm);
