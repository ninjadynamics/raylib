CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror

.PHONY: check clean

check: test_batch_default_f1.exe test_batch_stats.exe test_batch_f1_feature_zero.exe test_batch_abi_mismatch.exe test_batch_legacy.exe
	./test_batch_default_f1.exe
	./test_batch_stats.exe
	./test_batch_f1_feature_zero.exe
	./test_batch_abi_mismatch.exe
	./test_batch_legacy.exe

test_batch_default_f1.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=1 -o $@ test_batch.c

test_batch_stats.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=1 -DRLDC_ENABLE_STATS=1 -o $@ test_batch.c

test_batch_f1_feature_zero.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=0 -DAUDIT_GLKOS_INTERLEAVED_FEATURE_ZERO=1 -o $@ test_batch.c

test_batch_abi_mismatch.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=0 -DAUDIT_GLKOS_ABI_V1=1 -o $@ test_batch.c

test_batch_legacy.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_F1=0 -DAUDIT_GLKOS_LEGACY=1 -o $@ test_batch.c

clean:
	$(RM) test_batch_default_f1.exe test_batch_stats.exe test_batch_f1_feature_zero.exe test_batch_abi_mismatch.exe test_batch_legacy.exe
