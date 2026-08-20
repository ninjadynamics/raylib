CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror

.PHONY: check clean

check: test_batch_fast.exe test_batch_stats.exe test_batch_disabled.exe test_batch_feature_zero.exe test_batch_abi_mismatch.exe test_batch_legacy.exe
	./test_batch_fast.exe
	./test_batch_stats.exe
	./test_batch_disabled.exe
	./test_batch_feature_zero.exe
	./test_batch_abi_mismatch.exe
	./test_batch_legacy.exe

test_batch_fast.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_FAST=1 -o $@ test_batch.c

test_batch_stats.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_FAST=1 -DRLDC_ENABLE_STATS=1 -o $@ test_batch.c

test_batch_disabled.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_FAST=0 -DRLDC_USE_INTERLEAVED_FAST_PATH=0 -o $@ test_batch.c

test_batch_feature_zero.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_FAST=0 -DAUDIT_GLKOS_FEATURE_ZERO=1 -o $@ test_batch.c

test_batch_abi_mismatch.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_FAST=0 -DAUDIT_GLKOS_ABI_V2=1 -o $@ test_batch.c

test_batch_legacy.exe: test_batch.c GL/gl.h GL/glkos.h ../../src/rlgl_dc_batch.h
	$(CC) $(CFLAGS) -I. -DAUDIT_EXPECT_FAST=0 -DAUDIT_GLKOS_LEGACY=1 -o $@ test_batch.c

clean:
	$(RM) test_batch_fast.exe test_batch_stats.exe test_batch_disabled.exe test_batch_feature_zero.exe test_batch_abi_mismatch.exe test_batch_legacy.exe
