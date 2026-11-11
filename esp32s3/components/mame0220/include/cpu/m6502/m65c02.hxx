void m65c02_device::adc_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
case 4:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, X)) {

	if(icount == 0) { inst_substate = 3; return; }
		read(set_l(TMP, TMP+X));
	icount--;

	}

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, X)) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read(set_l(TMP, TMP+X));
	icount--;

	}

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_aby_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 3; return; }
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	TMP += Y;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_aby_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	TMP += Y;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_idx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP2);
	icount--;

	TMP2 += X;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	do_adc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_idx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP2);
	icount--;

	TMP2 += X;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	do_adc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
case 6:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_idy_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 4; return; }
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	if(icount == 0) { inst_substate = 5; return; }
	do_adc(read(TMP+Y));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_idy_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 4; return; }
case 4:
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	do_adc(read(TMP+Y));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
case 6:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_imm_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 2; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 3; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_imm_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 2; return; }
case 2:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 3; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP);
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	do_adc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	do_adc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::adc_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = read(uint8_t(TMP+X));
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::adc_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = read(uint8_t(TMP+X));
	icount--;

	do_adc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
case 4:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::and_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	A &= read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::and_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	A &= read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::asl_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::asl_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::asl_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::asl_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::asl_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::asl_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::asl_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::asl_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_asl(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::bbr_zpb_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = read_pc();
	icount--;

	if(!(TMP2 & (1 << ((inst_state >> 4) & 7)))) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		if(page_changing(PC, int8_t(TMP))) {

	if(icount == 0) { inst_substate = 6; return; }
			read_arg(set_l(PC, PC+int8_t(TMP)));
	icount--;

		}

		PC += int8_t(TMP);

	}

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::bbr_zpb_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = read_pc();
	icount--;

	if(!(TMP2 & (1 << ((inst_state >> 4) & 7)))) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		if(page_changing(PC, int8_t(TMP))) {

	if(icount == 0) { inst_substate = 6; return; }
case 6:
			read_arg(set_l(PC, PC+int8_t(TMP)));
	icount--;

		}

		PC += int8_t(TMP);

	}

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::bbs_zpb_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = read_pc();
	icount--;

	if(TMP2 & (1 << ((inst_state >> 4) & 7))) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		if(page_changing(PC, int8_t(TMP))) {

	if(icount == 0) { inst_substate = 6; return; }
			read_arg(set_l(PC, PC+int8_t(TMP)));
	icount--;

		}

		PC += int8_t(TMP);

	}

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::bbs_zpb_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = read_pc();
	icount--;

	if(TMP2 & (1 << ((inst_state >> 4) & 7))) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		if(page_changing(PC, int8_t(TMP))) {

	if(icount == 0) { inst_substate = 6; return; }
case 6:
			read_arg(set_l(PC, PC+int8_t(TMP)));
	icount--;

		}

		PC += int8_t(TMP);

	}

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::bit_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, X)) {

	if(icount == 0) { inst_substate = 3; return; }
		read(set_l(TMP, TMP+X));
	icount--;

	}

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	do_bit(read(TMP));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::bit_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, X)) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read(set_l(TMP, TMP+X));
	icount--;

	}

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	do_bit(read(TMP));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::bit_imm_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(A & TMP)

		P &= ~F_Z;

	else

		P |= F_Z;

	if(icount == 0) { inst_substate = 2; return; }
	prefetch();
	icount--;

}

void m65c02_device::bit_imm_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(A & TMP)

		P &= ~F_Z;

	else

		P |= F_Z;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::bit_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = read(uint8_t(TMP+X));
	icount--;

	do_bit(TMP);

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::bit_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = read(uint8_t(TMP+X));
	icount--;

	do_bit(TMP);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::bra_rel_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read_pc_noinc();
	icount--;

	if(page_changing(PC, int8_t(TMP))) {

	if(icount == 0) { inst_substate = 3; return; }
		read_arg(set_l(PC, PC+int8_t(TMP)));
	icount--;

	}

	PC += int8_t(TMP);

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::bra_rel_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read_pc_noinc();
	icount--;

	if(page_changing(PC, int8_t(TMP))) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read_arg(set_l(PC, PC+int8_t(TMP)));
	icount--;

	}

	PC += int8_t(TMP);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::brk_c_imp_full()
{

	if(irq_taken || nmi_state) {

	if(icount == 0) { inst_substate = 1; return; }
		read_pc_noinc();
	icount--;

	} else {

	if(icount == 0) { inst_substate = 2; return; }
		read_pc();
	icount--;

	}

	if(icount == 0) { inst_substate = 3; return; }
	write(SP, PC >> 8);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 4; return; }
	write(SP, PC);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 5; return; }
	write(SP, irq_taken || nmi_state ? P & ~F_B : P);
	icount--;

	dec_SP();

	if(irq_taken && nmi_state) {

	if(icount == 0) { inst_substate = 6; return; }
		PC = read_arg(0xfffa);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
		PC = set_h(PC, read_arg(0xfffb));
	icount--;

		nmi_state = false;

		standard_irq_callback(NMI_LINE);

	} else {

	if(icount == 0) { inst_substate = 8; return; }
		PC = read_arg(0xfffe);
	icount--;

	if(icount == 0) { inst_substate = 9; return; }
		PC = set_h(PC, read_arg(0xffff));
	icount--;

		if(irq_taken)

			standard_irq_callback(IRQ_LINE);

	}

	irq_taken = false;

	P = (P | F_I) & ~F_D; // Do *not* move after the prefetch

	if(icount == 0) { inst_substate = 10; return; }
	prefetch();
	icount--;

	inst_state = -1;

}

void m65c02_device::brk_c_imp_partial()
{
switch(inst_substate) {
case 0:

	if(irq_taken || nmi_state) {

	if(icount == 0) { inst_substate = 1; return; }
case 1:
		read_pc_noinc();
	icount--;

	} else {

	if(icount == 0) { inst_substate = 2; return; }
case 2:
		read_pc();
	icount--;

	}

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	write(SP, PC >> 8);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(SP, PC);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(SP, irq_taken || nmi_state ? P & ~F_B : P);
	icount--;

	dec_SP();

	if(irq_taken && nmi_state) {

	if(icount == 0) { inst_substate = 6; return; }
case 6:
		PC = read_arg(0xfffa);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
		PC = set_h(PC, read_arg(0xfffb));
	icount--;

		nmi_state = false;

		standard_irq_callback(NMI_LINE);

	} else {

	if(icount == 0) { inst_substate = 8; return; }
case 8:
		PC = read_arg(0xfffe);
	icount--;

	if(icount == 0) { inst_substate = 9; return; }
case 9:
		PC = set_h(PC, read_arg(0xffff));
	icount--;

		if(irq_taken)

			standard_irq_callback(IRQ_LINE);

	}

	irq_taken = false;

	P = (P | F_I) & ~F_D; // Do *not* move after the prefetch

	if(icount == 0) { inst_substate = 10; return; }
case 10:
	prefetch();
	icount--;

	inst_state = -1;

}
	inst_substate = 0;
}


void m65c02_device::cmp_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	do_cmp(A, read(TMP));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::cmp_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	do_cmp(A, read(TMP));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::dec_acc_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc_noinc();
	icount--;

	A--;

	set_nz(A);

	if(icount == 0) { inst_substate = 2; return; }
	prefetch();
	icount--;

}

void m65c02_device::dec_acc_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc_noinc();
	icount--;

	A--;

	set_nz(A);

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::dec_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::dec_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::dec_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::dec_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::dec_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::dec_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::dec_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::dec_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2--;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::eor_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	A ^= read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::eor_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	A ^= read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::inc_acc_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc_noinc();
	icount--;

	A++;

	set_nz(A);

	if(icount == 0) { inst_substate = 2; return; }
	prefetch();
	icount--;

}

void m65c02_device::inc_acc_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc_noinc();
	icount--;

	A++;

	set_nz(A);

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::inc_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::inc_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::inc_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::inc_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::inc_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::inc_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::inc_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::inc_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2++;

	set_nz(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::jmp_iax_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	PC = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	PC = set_h(PC, read(TMP+1));
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::jmp_iax_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	PC = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	PC = set_h(PC, read(TMP+1));
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::jmp_c_ind_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	PC = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(set_l(TMP, TMP+1));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	PC = set_h(PC, read(TMP+1));
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::jmp_c_ind_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	PC = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(set_l(TMP, TMP+1));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	PC = set_h(PC, read(TMP+1));
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::lda_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	A = read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::lda_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	A = read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::lsr_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::lsr_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::lsr_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::lsr_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::lsr_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::lsr_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::lsr_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::lsr_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_lsr(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::nop_c_imp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	prefetch();
	icount--;

}

void m65c02_device::nop_c_imp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::nop_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read_pc();
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 8; return; }
	prefetch();
	icount--;

}

void m65c02_device::nop_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read_pc();
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 8; return; }
case 8:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::nop_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::nop_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::ora_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	A |= read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::ora_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	A |= read(TMP);
	icount--;

	set_nz(A);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::phx_imp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	write(SP, X);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 3; return; }
	prefetch();
	icount--;

}

void m65c02_device::phx_imp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	write(SP, X);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::phy_imp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	write(SP, Y);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 3; return; }
	prefetch();
	icount--;

}

void m65c02_device::phy_imp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	write(SP, Y);
	icount--;

	dec_SP();

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::plx_imp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(SP);
	icount--;

	inc_SP();

	if(icount == 0) { inst_substate = 3; return; }
	X = read(SP);
	icount--;

	set_nz(X);

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::plx_imp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(SP);
	icount--;

	inc_SP();

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	X = read(SP);
	icount--;

	set_nz(X);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::ply_imp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(SP);
	icount--;

	inc_SP();

	if(icount == 0) { inst_substate = 3; return; }
	Y = read(SP);
	icount--;

	set_nz(Y);

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::ply_imp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(SP);
	icount--;

	inc_SP();

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	Y = read(SP);
	icount--;

	set_nz(Y);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::rmb_bzp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2 &= ~(1 << ((inst_state >> 4) & 7));

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::rmb_bzp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2 &= ~(1 << ((inst_state >> 4) & 7));

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::rol_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::rol_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::rol_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::rol_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::rol_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::rol_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::rol_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::rol_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_rol(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::ror_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::ror_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::ror_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::ror_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::ror_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::ror_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::ror_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::ror_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	TMP = uint8_t(TMP+X);

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	TMP2 = do_ror(TMP2);

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
case 4:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, X)) {

	if(icount == 0) { inst_substate = 3; return; }
		read(set_l(TMP, TMP+X));
	icount--;

	}

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, X)) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read(set_l(TMP, TMP+X));
	icount--;

	}

	TMP += X;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_aby_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 3; return; }
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	TMP += Y;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_aby_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	TMP += Y;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_idx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP2);
	icount--;

	TMP2 += X;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	do_sbc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_idx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP2);
	icount--;

	TMP2 += X;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	do_sbc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
case 6:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_idy_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 4; return; }
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	if(icount == 0) { inst_substate = 5; return; }
	do_sbc(read(TMP+Y));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_idy_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(page_changing(TMP, Y)) {

	if(icount == 0) { inst_substate = 4; return; }
case 4:
		read(set_l(TMP, TMP+Y));
	icount--;

	}

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	do_sbc(read(TMP+Y));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 6; return; }
case 6:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_imm_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 2; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 3; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_imm_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 2; return; }
case 2:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 3; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP);
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 3; return; }
case 3:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	do_sbc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	do_sbc(read(TMP));
	icount--;

	if(P & F_D) {

	if(icount == 0) { inst_substate = 5; return; }
case 5:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sbc_c_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = read(uint8_t(TMP+X));
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::sbc_c_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = read(uint8_t(TMP+X));
	icount--;

	do_sbc(TMP);

	if(P & F_D) {

	if(icount == 0) { inst_substate = 4; return; }
case 4:
		read_pc_noinc();
	icount--;

		set_nz(A);

	}

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::smb_bzp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	TMP2 |= 1 << ((inst_state >> 4) & 7);

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::smb_bzp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	TMP2 |= 1 << ((inst_state >> 4) & 7);

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::sta_zpi_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, A);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::sta_zpi_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP2 = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = read(TMP2 & 0xff);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP = set_h(TMP, read((TMP2+1) & 0xff));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, A);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::stp_imp_full()
{

	for(;;) {

	icount=0; inst_substate = 1; return;

	}

}

void m65c02_device::stp_imp_partial()
{
switch(inst_substate) {
case 0:

	for(;;) {

	icount=0; inst_substate = 1; return;
case 1:;

	}

}
	inst_substate = 0;
}


void m65c02_device::stz_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	write(TMP, 0x00);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::stz_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	write(TMP, 0x00);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::stz_abx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(set_l(TMP, TMP+X));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP+X, 0x00);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::stz_abx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(set_l(TMP, TMP+X));
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP+X, 0x00);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::stz_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	write(TMP, 0x00);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	prefetch();
	icount--;

}

void m65c02_device::stz_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	write(TMP, 0x00);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::stz_zpx_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	write(uint8_t(TMP+X), 0x00);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::stz_zpx_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	write(uint8_t(TMP+X), 0x00);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::trb_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 &= ~A;

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::trb_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 &= ~A;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::trb_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 &= ~A;

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::trb_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 &= ~A;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::tsb_aba_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 |= A;

	if(icount == 0) { inst_substate = 5; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
	prefetch();
	icount--;

}

void m65c02_device::tsb_aba_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP = set_h(TMP, read_pc());
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 |= A;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::tsb_zpg_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 |= A;

	if(icount == 0) { inst_substate = 4; return; }
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	prefetch();
	icount--;

}

void m65c02_device::tsb_zpg_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	TMP = read_pc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	TMP2 = read(TMP);
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(TMP);
	icount--;

	if(A & TMP2)

		P &= ~F_Z;

	else

		P |= F_Z;

	TMP2 |= A;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	write(TMP, TMP2);
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::wai_imp_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read_pc_noinc();
	icount--;

	while(!nmi_state && !irq_state) {

	icount=0; inst_substate = 3; return;

	}

	if(icount == 0) { inst_substate = 4; return; }
	prefetch();
	icount--;

}

void m65c02_device::wai_imp_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read_pc_noinc();
	icount--;

	while(!nmi_state && !irq_state) {

	icount=0; inst_substate = 3; return;
case 3:;

	}

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	prefetch();
	icount--;

}
	inst_substate = 0;
}


void m65c02_device::reset_c_full()
{

	if(icount == 0) { inst_substate = 1; return; }
	read_arg(0xffff);
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
	read(SP); dec_SP();
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
	read(SP); dec_SP();
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
	read(SP); dec_SP();
	icount--;

	P = (P | F_I) & ~F_D;

	if(icount == 0) { inst_substate = 6; return; }
	PC = read_arg(0xfffc);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
	PC = set_h(PC, read_arg(0xfffd));
	icount--;

	if(icount == 0) { inst_substate = 8; return; }
	prefetch();
	icount--;

	inst_state = -1;

}

void m65c02_device::reset_c_partial()
{
switch(inst_substate) {
case 0:

	if(icount == 0) { inst_substate = 1; return; }
case 1:
	read_arg(0xffff);
	icount--;

	if(icount == 0) { inst_substate = 2; return; }
case 2:
	read_pc_noinc();
	icount--;

	if(icount == 0) { inst_substate = 3; return; }
case 3:
	read(SP); dec_SP();
	icount--;

	if(icount == 0) { inst_substate = 4; return; }
case 4:
	read(SP); dec_SP();
	icount--;

	if(icount == 0) { inst_substate = 5; return; }
case 5:
	read(SP); dec_SP();
	icount--;

	P = (P | F_I) & ~F_D;

	if(icount == 0) { inst_substate = 6; return; }
case 6:
	PC = read_arg(0xfffc);
	icount--;

	if(icount == 0) { inst_substate = 7; return; }
case 7:
	PC = set_h(PC, read_arg(0xfffd));
	icount--;

	if(icount == 0) { inst_substate = 8; return; }
case 8:
	prefetch();
	icount--;

	inst_state = -1;

}
	inst_substate = 0;
}




void m65c02_device::do_exec_full()
{
	switch(inst_state) {

	case 0x00: brk_c_imp_full(); break;
	case 0x01: ora_idx_full(); break;
	case 0x02: nop_imm_full(); break;
	case 0x03: nop_c_imp_full(); break;
	case 0x04: tsb_zpg_full(); break;
	case 0x05: ora_zpg_full(); break;
	case 0x06: asl_c_zpg_full(); break;
	case 0x07: nop_c_imp_full(); break;
	case 0x08: php_imp_full(); break;
	case 0x09: ora_imm_full(); break;
	case 0x0a: asl_acc_full(); break;
	case 0x0b: nop_c_imp_full(); break;
	case 0x0c: tsb_aba_full(); break;
	case 0x0d: ora_aba_full(); break;
	case 0x0e: asl_c_aba_full(); break;
	case 0x0f: nop_c_imp_full(); break;
	case 0x10: bpl_rel_full(); break;
	case 0x11: ora_idy_full(); break;
	case 0x12: ora_zpi_full(); break;
	case 0x13: nop_c_imp_full(); break;
	case 0x14: trb_zpg_full(); break;
	case 0x15: ora_zpx_full(); break;
	case 0x16: asl_c_zpx_full(); break;
	case 0x17: nop_c_imp_full(); break;
	case 0x18: clc_imp_full(); break;
	case 0x19: ora_aby_full(); break;
	case 0x1a: inc_acc_full(); break;
	case 0x1b: nop_c_imp_full(); break;
	case 0x1c: trb_aba_full(); break;
	case 0x1d: ora_abx_full(); break;
	case 0x1e: asl_c_abx_full(); break;
	case 0x1f: nop_c_imp_full(); break;
	case 0x20: jsr_adr_full(); break;
	case 0x21: and_idx_full(); break;
	case 0x22: nop_imm_full(); break;
	case 0x23: nop_c_imp_full(); break;
	case 0x24: bit_zpg_full(); break;
	case 0x25: and_zpg_full(); break;
	case 0x26: rol_c_zpg_full(); break;
	case 0x27: nop_c_imp_full(); break;
	case 0x28: plp_imp_full(); break;
	case 0x29: and_imm_full(); break;
	case 0x2a: rol_acc_full(); break;
	case 0x2b: nop_c_imp_full(); break;
	case 0x2c: bit_aba_full(); break;
	case 0x2d: and_aba_full(); break;
	case 0x2e: rol_c_aba_full(); break;
	case 0x2f: nop_c_imp_full(); break;
	case 0x30: bmi_rel_full(); break;
	case 0x31: and_idy_full(); break;
	case 0x32: and_zpi_full(); break;
	case 0x33: nop_c_imp_full(); break;
	case 0x34: bit_zpx_full(); break;
	case 0x35: and_zpx_full(); break;
	case 0x36: rol_c_zpx_full(); break;
	case 0x37: nop_c_imp_full(); break;
	case 0x38: sec_imp_full(); break;
	case 0x39: and_aby_full(); break;
	case 0x3a: dec_acc_full(); break;
	case 0x3b: nop_c_imp_full(); break;
	case 0x3c: bit_abx_full(); break;
	case 0x3d: and_abx_full(); break;
	case 0x3e: rol_c_abx_full(); break;
	case 0x3f: nop_c_imp_full(); break;
	case 0x40: rti_imp_full(); break;
	case 0x41: eor_idx_full(); break;
	case 0x42: nop_imm_full(); break;
	case 0x43: nop_c_imp_full(); break;
	case 0x44: nop_zpg_full(); break;
	case 0x45: eor_zpg_full(); break;
	case 0x46: lsr_c_zpg_full(); break;
	case 0x47: nop_c_imp_full(); break;
	case 0x48: pha_imp_full(); break;
	case 0x49: eor_imm_full(); break;
	case 0x4a: lsr_acc_full(); break;
	case 0x4b: nop_c_imp_full(); break;
	case 0x4c: jmp_adr_full(); break;
	case 0x4d: eor_aba_full(); break;
	case 0x4e: lsr_c_aba_full(); break;
	case 0x4f: nop_c_imp_full(); break;
	case 0x50: bvc_rel_full(); break;
	case 0x51: eor_idy_full(); break;
	case 0x52: eor_zpi_full(); break;
	case 0x53: nop_c_imp_full(); break;
	case 0x54: nop_zpx_full(); break;
	case 0x55: eor_zpx_full(); break;
	case 0x56: lsr_c_zpx_full(); break;
	case 0x57: nop_c_imp_full(); break;
	case 0x58: cli_imp_full(); break;
	case 0x59: eor_aby_full(); break;
	case 0x5a: phy_imp_full(); break;
	case 0x5b: nop_c_imp_full(); break;
	case 0x5c: nop_c_aba_full(); break;
	case 0x5d: eor_abx_full(); break;
	case 0x5e: lsr_c_abx_full(); break;
	case 0x5f: nop_c_imp_full(); break;
	case 0x60: rts_imp_full(); break;
	case 0x61: adc_c_idx_full(); break;
	case 0x62: nop_imm_full(); break;
	case 0x63: nop_c_imp_full(); break;
	case 0x64: stz_zpg_full(); break;
	case 0x65: adc_c_zpg_full(); break;
	case 0x66: ror_c_zpg_full(); break;
	case 0x67: nop_c_imp_full(); break;
	case 0x68: pla_imp_full(); break;
	case 0x69: adc_c_imm_full(); break;
	case 0x6a: ror_acc_full(); break;
	case 0x6b: nop_c_imp_full(); break;
	case 0x6c: jmp_c_ind_full(); break;
	case 0x6d: adc_c_aba_full(); break;
	case 0x6e: ror_c_aba_full(); break;
	case 0x6f: nop_c_imp_full(); break;
	case 0x70: bvs_rel_full(); break;
	case 0x71: adc_c_idy_full(); break;
	case 0x72: adc_c_zpi_full(); break;
	case 0x73: nop_c_imp_full(); break;
	case 0x74: stz_zpx_full(); break;
	case 0x75: adc_c_zpx_full(); break;
	case 0x76: ror_c_zpx_full(); break;
	case 0x77: nop_c_imp_full(); break;
	case 0x78: sei_imp_full(); break;
	case 0x79: adc_c_aby_full(); break;
	case 0x7a: ply_imp_full(); break;
	case 0x7b: nop_c_imp_full(); break;
	case 0x7c: jmp_iax_full(); break;
	case 0x7d: adc_c_abx_full(); break;
	case 0x7e: ror_c_abx_full(); break;
	case 0x7f: nop_c_imp_full(); break;
	case 0x80: bra_rel_full(); break;
	case 0x81: sta_idx_full(); break;
	case 0x82: nop_imm_full(); break;
	case 0x83: nop_c_imp_full(); break;
	case 0x84: sty_zpg_full(); break;
	case 0x85: sta_zpg_full(); break;
	case 0x86: stx_zpg_full(); break;
	case 0x87: nop_c_imp_full(); break;
	case 0x88: dey_imp_full(); break;
	case 0x89: bit_imm_full(); break;
	case 0x8a: txa_imp_full(); break;
	case 0x8b: nop_c_imp_full(); break;
	case 0x8c: sty_aba_full(); break;
	case 0x8d: sta_aba_full(); break;
	case 0x8e: stx_aba_full(); break;
	case 0x8f: nop_c_imp_full(); break;
	case 0x90: bcc_rel_full(); break;
	case 0x91: sta_idy_full(); break;
	case 0x92: sta_zpi_full(); break;
	case 0x93: nop_c_imp_full(); break;
	case 0x94: sty_zpx_full(); break;
	case 0x95: sta_zpx_full(); break;
	case 0x96: stx_zpy_full(); break;
	case 0x97: nop_c_imp_full(); break;
	case 0x98: tya_imp_full(); break;
	case 0x99: sta_aby_full(); break;
	case 0x9a: txs_imp_full(); break;
	case 0x9b: nop_c_imp_full(); break;
	case 0x9c: stz_aba_full(); break;
	case 0x9d: sta_abx_full(); break;
	case 0x9e: stz_abx_full(); break;
	case 0x9f: nop_c_imp_full(); break;
	case 0xa0: ldy_imm_full(); break;
	case 0xa1: lda_idx_full(); break;
	case 0xa2: ldx_imm_full(); break;
	case 0xa3: nop_c_imp_full(); break;
	case 0xa4: ldy_zpg_full(); break;
	case 0xa5: lda_zpg_full(); break;
	case 0xa6: ldx_zpg_full(); break;
	case 0xa7: nop_c_imp_full(); break;
	case 0xa8: tay_imp_full(); break;
	case 0xa9: lda_imm_full(); break;
	case 0xaa: tax_imp_full(); break;
	case 0xab: nop_c_imp_full(); break;
	case 0xac: ldy_aba_full(); break;
	case 0xad: lda_aba_full(); break;
	case 0xae: ldx_aba_full(); break;
	case 0xaf: nop_c_imp_full(); break;
	case 0xb0: bcs_rel_full(); break;
	case 0xb1: lda_idy_full(); break;
	case 0xb2: lda_zpi_full(); break;
	case 0xb3: nop_c_imp_full(); break;
	case 0xb4: ldy_zpx_full(); break;
	case 0xb5: lda_zpx_full(); break;
	case 0xb6: ldx_zpy_full(); break;
	case 0xb7: nop_c_imp_full(); break;
	case 0xb8: clv_imp_full(); break;
	case 0xb9: lda_aby_full(); break;
	case 0xba: tsx_imp_full(); break;
	case 0xbb: nop_c_imp_full(); break;
	case 0xbc: ldy_abx_full(); break;
	case 0xbd: lda_abx_full(); break;
	case 0xbe: ldx_aby_full(); break;
	case 0xbf: nop_c_imp_full(); break;
	case 0xc0: cpy_imm_full(); break;
	case 0xc1: cmp_idx_full(); break;
	case 0xc2: nop_imm_full(); break;
	case 0xc3: nop_c_imp_full(); break;
	case 0xc4: cpy_zpg_full(); break;
	case 0xc5: cmp_zpg_full(); break;
	case 0xc6: dec_c_zpg_full(); break;
	case 0xc7: nop_c_imp_full(); break;
	case 0xc8: iny_imp_full(); break;
	case 0xc9: cmp_imm_full(); break;
	case 0xca: dex_imp_full(); break;
	case 0xcb: nop_c_imp_full(); break;
	case 0xcc: cpy_aba_full(); break;
	case 0xcd: cmp_aba_full(); break;
	case 0xce: dec_c_aba_full(); break;
	case 0xcf: nop_c_imp_full(); break;
	case 0xd0: bne_rel_full(); break;
	case 0xd1: cmp_idy_full(); break;
	case 0xd2: cmp_zpi_full(); break;
	case 0xd3: nop_c_imp_full(); break;
	case 0xd4: nop_zpx_full(); break;
	case 0xd5: cmp_zpx_full(); break;
	case 0xd6: dec_c_zpx_full(); break;
	case 0xd7: nop_c_imp_full(); break;
	case 0xd8: cld_imp_full(); break;
	case 0xd9: cmp_aby_full(); break;
	case 0xda: phx_imp_full(); break;
	case 0xdb: nop_c_imp_full(); break;
	case 0xdc: nop_c_abx_full(); break;
	case 0xdd: cmp_abx_full(); break;
	case 0xde: dec_c_abx_full(); break;
	case 0xdf: nop_c_imp_full(); break;
	case 0xe0: cpx_imm_full(); break;
	case 0xe1: sbc_c_idx_full(); break;
	case 0xe2: nop_imm_full(); break;
	case 0xe3: nop_c_imp_full(); break;
	case 0xe4: cpx_zpg_full(); break;
	case 0xe5: sbc_c_zpg_full(); break;
	case 0xe6: inc_c_zpg_full(); break;
	case 0xe7: nop_c_imp_full(); break;
	case 0xe8: inx_imp_full(); break;
	case 0xe9: sbc_c_imm_full(); break;
	case 0xea: nop_imp_full(); break;
	case 0xeb: nop_c_imp_full(); break;
	case 0xec: cpx_aba_full(); break;
	case 0xed: sbc_c_aba_full(); break;
	case 0xee: inc_c_aba_full(); break;
	case 0xef: nop_c_imp_full(); break;
	case 0xf0: beq_rel_full(); break;
	case 0xf1: sbc_c_idy_full(); break;
	case 0xf2: sbc_c_zpi_full(); break;
	case 0xf3: nop_c_imp_full(); break;
	case 0xf4: nop_zpx_full(); break;
	case 0xf5: sbc_c_zpx_full(); break;
	case 0xf6: inc_c_zpx_full(); break;
	case 0xf7: nop_c_imp_full(); break;
	case 0xf8: sed_imp_full(); break;
	case 0xf9: sbc_c_aby_full(); break;
	case 0xfa: plx_imp_full(); break;
	case 0xfb: nop_c_imp_full(); break;
	case 0xfc: nop_c_abx_full(); break;
	case 0xfd: sbc_c_abx_full(); break;
	case 0xfe: inc_c_abx_full(); break;
	case 0xff: nop_c_imp_full(); break;
	case STATE_RESET: reset_c_full(); break;
	}
}

void m65c02_device::do_exec_partial()
{
	switch(inst_state) {

	case 0x00: brk_c_imp_partial(); break;
	case 0x01: ora_idx_partial(); break;
	case 0x02: nop_imm_partial(); break;
	case 0x03: nop_c_imp_partial(); break;
	case 0x04: tsb_zpg_partial(); break;
	case 0x05: ora_zpg_partial(); break;
	case 0x06: asl_c_zpg_partial(); break;
	case 0x07: nop_c_imp_partial(); break;
	case 0x08: php_imp_partial(); break;
	case 0x09: ora_imm_partial(); break;
	case 0x0a: asl_acc_partial(); break;
	case 0x0b: nop_c_imp_partial(); break;
	case 0x0c: tsb_aba_partial(); break;
	case 0x0d: ora_aba_partial(); break;
	case 0x0e: asl_c_aba_partial(); break;
	case 0x0f: nop_c_imp_partial(); break;
	case 0x10: bpl_rel_partial(); break;
	case 0x11: ora_idy_partial(); break;
	case 0x12: ora_zpi_partial(); break;
	case 0x13: nop_c_imp_partial(); break;
	case 0x14: trb_zpg_partial(); break;
	case 0x15: ora_zpx_partial(); break;
	case 0x16: asl_c_zpx_partial(); break;
	case 0x17: nop_c_imp_partial(); break;
	case 0x18: clc_imp_partial(); break;
	case 0x19: ora_aby_partial(); break;
	case 0x1a: inc_acc_partial(); break;
	case 0x1b: nop_c_imp_partial(); break;
	case 0x1c: trb_aba_partial(); break;
	case 0x1d: ora_abx_partial(); break;
	case 0x1e: asl_c_abx_partial(); break;
	case 0x1f: nop_c_imp_partial(); break;
	case 0x20: jsr_adr_partial(); break;
	case 0x21: and_idx_partial(); break;
	case 0x22: nop_imm_partial(); break;
	case 0x23: nop_c_imp_partial(); break;
	case 0x24: bit_zpg_partial(); break;
	case 0x25: and_zpg_partial(); break;
	case 0x26: rol_c_zpg_partial(); break;
	case 0x27: nop_c_imp_partial(); break;
	case 0x28: plp_imp_partial(); break;
	case 0x29: and_imm_partial(); break;
	case 0x2a: rol_acc_partial(); break;
	case 0x2b: nop_c_imp_partial(); break;
	case 0x2c: bit_aba_partial(); break;
	case 0x2d: and_aba_partial(); break;
	case 0x2e: rol_c_aba_partial(); break;
	case 0x2f: nop_c_imp_partial(); break;
	case 0x30: bmi_rel_partial(); break;
	case 0x31: and_idy_partial(); break;
	case 0x32: and_zpi_partial(); break;
	case 0x33: nop_c_imp_partial(); break;
	case 0x34: bit_zpx_partial(); break;
	case 0x35: and_zpx_partial(); break;
	case 0x36: rol_c_zpx_partial(); break;
	case 0x37: nop_c_imp_partial(); break;
	case 0x38: sec_imp_partial(); break;
	case 0x39: and_aby_partial(); break;
	case 0x3a: dec_acc_partial(); break;
	case 0x3b: nop_c_imp_partial(); break;
	case 0x3c: bit_abx_partial(); break;
	case 0x3d: and_abx_partial(); break;
	case 0x3e: rol_c_abx_partial(); break;
	case 0x3f: nop_c_imp_partial(); break;
	case 0x40: rti_imp_partial(); break;
	case 0x41: eor_idx_partial(); break;
	case 0x42: nop_imm_partial(); break;
	case 0x43: nop_c_imp_partial(); break;
	case 0x44: nop_zpg_partial(); break;
	case 0x45: eor_zpg_partial(); break;
	case 0x46: lsr_c_zpg_partial(); break;
	case 0x47: nop_c_imp_partial(); break;
	case 0x48: pha_imp_partial(); break;
	case 0x49: eor_imm_partial(); break;
	case 0x4a: lsr_acc_partial(); break;
	case 0x4b: nop_c_imp_partial(); break;
	case 0x4c: jmp_adr_partial(); break;
	case 0x4d: eor_aba_partial(); break;
	case 0x4e: lsr_c_aba_partial(); break;
	case 0x4f: nop_c_imp_partial(); break;
	case 0x50: bvc_rel_partial(); break;
	case 0x51: eor_idy_partial(); break;
	case 0x52: eor_zpi_partial(); break;
	case 0x53: nop_c_imp_partial(); break;
	case 0x54: nop_zpx_partial(); break;
	case 0x55: eor_zpx_partial(); break;
	case 0x56: lsr_c_zpx_partial(); break;
	case 0x57: nop_c_imp_partial(); break;
	case 0x58: cli_imp_partial(); break;
	case 0x59: eor_aby_partial(); break;
	case 0x5a: phy_imp_partial(); break;
	case 0x5b: nop_c_imp_partial(); break;
	case 0x5c: nop_c_aba_partial(); break;
	case 0x5d: eor_abx_partial(); break;
	case 0x5e: lsr_c_abx_partial(); break;
	case 0x5f: nop_c_imp_partial(); break;
	case 0x60: rts_imp_partial(); break;
	case 0x61: adc_c_idx_partial(); break;
	case 0x62: nop_imm_partial(); break;
	case 0x63: nop_c_imp_partial(); break;
	case 0x64: stz_zpg_partial(); break;
	case 0x65: adc_c_zpg_partial(); break;
	case 0x66: ror_c_zpg_partial(); break;
	case 0x67: nop_c_imp_partial(); break;
	case 0x68: pla_imp_partial(); break;
	case 0x69: adc_c_imm_partial(); break;
	case 0x6a: ror_acc_partial(); break;
	case 0x6b: nop_c_imp_partial(); break;
	case 0x6c: jmp_c_ind_partial(); break;
	case 0x6d: adc_c_aba_partial(); break;
	case 0x6e: ror_c_aba_partial(); break;
	case 0x6f: nop_c_imp_partial(); break;
	case 0x70: bvs_rel_partial(); break;
	case 0x71: adc_c_idy_partial(); break;
	case 0x72: adc_c_zpi_partial(); break;
	case 0x73: nop_c_imp_partial(); break;
	case 0x74: stz_zpx_partial(); break;
	case 0x75: adc_c_zpx_partial(); break;
	case 0x76: ror_c_zpx_partial(); break;
	case 0x77: nop_c_imp_partial(); break;
	case 0x78: sei_imp_partial(); break;
	case 0x79: adc_c_aby_partial(); break;
	case 0x7a: ply_imp_partial(); break;
	case 0x7b: nop_c_imp_partial(); break;
	case 0x7c: jmp_iax_partial(); break;
	case 0x7d: adc_c_abx_partial(); break;
	case 0x7e: ror_c_abx_partial(); break;
	case 0x7f: nop_c_imp_partial(); break;
	case 0x80: bra_rel_partial(); break;
	case 0x81: sta_idx_partial(); break;
	case 0x82: nop_imm_partial(); break;
	case 0x83: nop_c_imp_partial(); break;
	case 0x84: sty_zpg_partial(); break;
	case 0x85: sta_zpg_partial(); break;
	case 0x86: stx_zpg_partial(); break;
	case 0x87: nop_c_imp_partial(); break;
	case 0x88: dey_imp_partial(); break;
	case 0x89: bit_imm_partial(); break;
	case 0x8a: txa_imp_partial(); break;
	case 0x8b: nop_c_imp_partial(); break;
	case 0x8c: sty_aba_partial(); break;
	case 0x8d: sta_aba_partial(); break;
	case 0x8e: stx_aba_partial(); break;
	case 0x8f: nop_c_imp_partial(); break;
	case 0x90: bcc_rel_partial(); break;
	case 0x91: sta_idy_partial(); break;
	case 0x92: sta_zpi_partial(); break;
	case 0x93: nop_c_imp_partial(); break;
	case 0x94: sty_zpx_partial(); break;
	case 0x95: sta_zpx_partial(); break;
	case 0x96: stx_zpy_partial(); break;
	case 0x97: nop_c_imp_partial(); break;
	case 0x98: tya_imp_partial(); break;
	case 0x99: sta_aby_partial(); break;
	case 0x9a: txs_imp_partial(); break;
	case 0x9b: nop_c_imp_partial(); break;
	case 0x9c: stz_aba_partial(); break;
	case 0x9d: sta_abx_partial(); break;
	case 0x9e: stz_abx_partial(); break;
	case 0x9f: nop_c_imp_partial(); break;
	case 0xa0: ldy_imm_partial(); break;
	case 0xa1: lda_idx_partial(); break;
	case 0xa2: ldx_imm_partial(); break;
	case 0xa3: nop_c_imp_partial(); break;
	case 0xa4: ldy_zpg_partial(); break;
	case 0xa5: lda_zpg_partial(); break;
	case 0xa6: ldx_zpg_partial(); break;
	case 0xa7: nop_c_imp_partial(); break;
	case 0xa8: tay_imp_partial(); break;
	case 0xa9: lda_imm_partial(); break;
	case 0xaa: tax_imp_partial(); break;
	case 0xab: nop_c_imp_partial(); break;
	case 0xac: ldy_aba_partial(); break;
	case 0xad: lda_aba_partial(); break;
	case 0xae: ldx_aba_partial(); break;
	case 0xaf: nop_c_imp_partial(); break;
	case 0xb0: bcs_rel_partial(); break;
	case 0xb1: lda_idy_partial(); break;
	case 0xb2: lda_zpi_partial(); break;
	case 0xb3: nop_c_imp_partial(); break;
	case 0xb4: ldy_zpx_partial(); break;
	case 0xb5: lda_zpx_partial(); break;
	case 0xb6: ldx_zpy_partial(); break;
	case 0xb7: nop_c_imp_partial(); break;
	case 0xb8: clv_imp_partial(); break;
	case 0xb9: lda_aby_partial(); break;
	case 0xba: tsx_imp_partial(); break;
	case 0xbb: nop_c_imp_partial(); break;
	case 0xbc: ldy_abx_partial(); break;
	case 0xbd: lda_abx_partial(); break;
	case 0xbe: ldx_aby_partial(); break;
	case 0xbf: nop_c_imp_partial(); break;
	case 0xc0: cpy_imm_partial(); break;
	case 0xc1: cmp_idx_partial(); break;
	case 0xc2: nop_imm_partial(); break;
	case 0xc3: nop_c_imp_partial(); break;
	case 0xc4: cpy_zpg_partial(); break;
	case 0xc5: cmp_zpg_partial(); break;
	case 0xc6: dec_c_zpg_partial(); break;
	case 0xc7: nop_c_imp_partial(); break;
	case 0xc8: iny_imp_partial(); break;
	case 0xc9: cmp_imm_partial(); break;
	case 0xca: dex_imp_partial(); break;
	case 0xcb: nop_c_imp_partial(); break;
	case 0xcc: cpy_aba_partial(); break;
	case 0xcd: cmp_aba_partial(); break;
	case 0xce: dec_c_aba_partial(); break;
	case 0xcf: nop_c_imp_partial(); break;
	case 0xd0: bne_rel_partial(); break;
	case 0xd1: cmp_idy_partial(); break;
	case 0xd2: cmp_zpi_partial(); break;
	case 0xd3: nop_c_imp_partial(); break;
	case 0xd4: nop_zpx_partial(); break;
	case 0xd5: cmp_zpx_partial(); break;
	case 0xd6: dec_c_zpx_partial(); break;
	case 0xd7: nop_c_imp_partial(); break;
	case 0xd8: cld_imp_partial(); break;
	case 0xd9: cmp_aby_partial(); break;
	case 0xda: phx_imp_partial(); break;
	case 0xdb: nop_c_imp_partial(); break;
	case 0xdc: nop_c_abx_partial(); break;
	case 0xdd: cmp_abx_partial(); break;
	case 0xde: dec_c_abx_partial(); break;
	case 0xdf: nop_c_imp_partial(); break;
	case 0xe0: cpx_imm_partial(); break;
	case 0xe1: sbc_c_idx_partial(); break;
	case 0xe2: nop_imm_partial(); break;
	case 0xe3: nop_c_imp_partial(); break;
	case 0xe4: cpx_zpg_partial(); break;
	case 0xe5: sbc_c_zpg_partial(); break;
	case 0xe6: inc_c_zpg_partial(); break;
	case 0xe7: nop_c_imp_partial(); break;
	case 0xe8: inx_imp_partial(); break;
	case 0xe9: sbc_c_imm_partial(); break;
	case 0xea: nop_imp_partial(); break;
	case 0xeb: nop_c_imp_partial(); break;
	case 0xec: cpx_aba_partial(); break;
	case 0xed: sbc_c_aba_partial(); break;
	case 0xee: inc_c_aba_partial(); break;
	case 0xef: nop_c_imp_partial(); break;
	case 0xf0: beq_rel_partial(); break;
	case 0xf1: sbc_c_idy_partial(); break;
	case 0xf2: sbc_c_zpi_partial(); break;
	case 0xf3: nop_c_imp_partial(); break;
	case 0xf4: nop_zpx_partial(); break;
	case 0xf5: sbc_c_zpx_partial(); break;
	case 0xf6: inc_c_zpx_partial(); break;
	case 0xf7: nop_c_imp_partial(); break;
	case 0xf8: sed_imp_partial(); break;
	case 0xf9: sbc_c_aby_partial(); break;
	case 0xfa: plx_imp_partial(); break;
	case 0xfb: nop_c_imp_partial(); break;
	case 0xfc: nop_c_abx_partial(); break;
	case 0xfd: sbc_c_abx_partial(); break;
	case 0xfe: inc_c_abx_partial(); break;
	case 0xff: nop_c_imp_partial(); break;
	case STATE_RESET: reset_c_partial(); break;
	}
}

