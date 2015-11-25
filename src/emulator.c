#include <emulator.h>

int main() {

    chip8 chip;

    initChip(&chip);
    runChip(&chip);
    return 0;
}

void initChip(chip8* chip) {
    memset(chip->memory, 0, CHIP_MEMORY*sizeof(unsigned char));
    memset(chip->v_reg, 0, V_REGISTERS*sizeof(unsigned char));
    chip->I = 0;
    chip->pc = 0;
    chip->opcode = 0;
    memset(chip->stack, 0, STACK_SIZE*sizeof(unsigned int));
    chip->stack_ptr = 0;
    chip->delay_timer = 0;
    chip->sound_timer = 0;
    memset(chip->keys, 0, KEYS*sizeof(unsigned char));
    memset(chip->display, 0, DISPLAY_SIZE*sizeof(unsigned char));
    chip->display_width = DISPLAY_WIDTH;
    chip->display_height = DISPLAY_HEIGHT;
}

void runChip(chip8* chip) {

    //fetch opcode
    chip->opcode = (chip->memory[chip->pc] << 8) | chip->memory[chip->pc+1];

    //decode opcode
    switch(chip->opcode & 0xF000) {
        int auxiliary_var;
        case 0x0000:
            switch(chip->opcode & 0x00FF) {
                case 0x00E0:    //00E0, clears the screen
                    memset(chip->display, 0, DISPLAY_SIZE*sizeof(unsigned char));
                    chip->pc += 2;
                break;
                case 0x00EE:    //00EE, returns from a subroutine
                    chip->pc = chip->stack[(--chip->stack_ptr)&0xF] + 2;
                break;
                default:        //0NNN, calls RCA 1802 program at address NNN
                    //TODO implement this opcode
                    chip->pc += 2;
                break;
            }
        case 0x1000:        //1NNN, jumps to address NNN
            chip->pc = chip->opcode & 0x0FFF;
        break;
        case 0x2000:        //2NNN, calls subroutine at NNN
            chip->stack[(chip->stack_ptr++)&0xF] = chip->pc;
            chip->pc = chip->opcode & 0x0FFF;
        break;
        case 0x3000:        //3XNN, skips the next instruction if VX equals NN
            if(chip->v_reg[(chip->opcode & 0x0F00) >> 8] == (chip->opcode & 0x00FF)) {
                chip->pc += 4;
            } else {
                chip->pc += 2;
            }
        break;
        case 0x4000:        //4XNN, skips the next instruction if VX doesn't equal NN
            if(chip->v_reg[(chip->opcode & 0x0F00) >> 8] != (chip->opcode & 0x00FF)) {
                chip->pc += 4;
            } else {
                chip->pc += 2;
            }
        break;
        case 0x5000:        //5XY0, skips the next instruction if VX equals VY
            if(chip->v_reg[(chip->opcode & 0x0F00) >> 8] == chip->v_reg[(chip->opcode & 0x00F0) >> 4]) {
                chip->pc += 4;
            } else {
                chip->pc += 2;
            }
        break;
        case 0x6000:        //6XNN, sets VX to NN
            chip->v_reg[(chip->opcode & 0x0F00) >> 8] = (chip->opcode & 0x00FF);
            chip->pc += 2;
        break;
        case 0x7000:        //7XNN, adds NN to VX
            chip->v_reg[(chip->opcode & 0x0F00) >> 8] += (chip->opcode & 0x00FF);
            chip->pc += 2;
        break;
        case 0x8000:
            switch(chip->opcode & 0x000F) {
                case 0x0000:        //8XY0, sets VX to the value of VY
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = chip->v_reg[(chip->opcode & 0x00F0) >> 4];
                    chip->pc += 2;
                break;
                case 0x0001:        //8XY1, sets VX to VX or VY
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = chip->v_reg[(chip->opcode & 0x0F00) >> 8] | chip->v_reg[(chip->opcode & 0x00F0) >> 4];
                    chip->pc += 2;
                break;
                case 0x0002:        //8XY2, sets VX to VX and VY
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = chip->v_reg[(chip->opcode & 0x0F00) >> 8] & chip->v_reg[(chip->opcode & 0x00F0) >> 4];
                    chip->pc += 2;
                break;
                case 0x0003:        //8XY3, sets VX to VX xor VY
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = chip->v_reg[(chip->opcode & 0x0F00) >> 8] ^ chip->v_reg[(chip->opcode & 0x00F0) >> 4];
                    chip->pc += 2;
                break;
                case 0x0004:        //8XY4, adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't
                    auxiliary_var = (int)chip->v_reg[(chip->opcode & 0x0F00) >> 8 ] + (int)chip->v_reg[(chip->opcode & 0x00F0) >> 4];
                    if(auxiliary_var < 256) {
                        chip->v_reg[0xF] &= 0;
                    } else {
                        chip->v_reg[0xF] = 1;
                    }
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = auxiliary_var & 0xF;
                    chip->pc += 2;
                break;
                case 0x0005:        //8XY5, VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                    auxiliary_var = (int)chip->v_reg[(chip->opcode & 0x0F00) >> 8 ] - (int)chip->v_reg[(chip->opcode & 0x00F0) >> 4];
                    if(auxiliary_var >= 0) {
                        chip->v_reg[0xF] = 1;
                    } else {
                        chip->v_reg[0xF] &= 0;
                    }
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = auxiliary_var & 0xF;
                    chip->pc += 2;
                break;
                case 0x0006:        //8XY6, shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift
                    chip->v_reg[0xF] = chip->v_reg[(chip->opcode & 0x0F00) >> 8] & 7;
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = chip->v_reg[(chip->opcode & 0x0F00) >> 8] >> 1;
                    chip->pc += 2;
                break;
                case 0x0007:        //8XY7, sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                    auxiliary_var = (int)chip->v_reg[(chip->opcode & 0x00F0) >> 4] - (int)chip->v_reg[(chip->opcode & 0x0F00) >> 8];
                    if(auxiliary_var >= 0) {
                        chip->v_reg[0xF] = 1;
                    } else {
                        chip->v_reg[0xF] &= 0;
                    }
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = auxiliary_var & 0xF;
                    chip->pc += 2;
                break;
                case 0x000E:        //8XYE, shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift
                    chip->v_reg[0xF] = chip->v_reg[(chip->opcode & 0x0F00) >> 8] >> 7;
                    chip->v_reg[(chip->opcode & 0x0F00) >> 8] = chip->v_reg[(chip->opcode & 0x0F00) >> 8] << 1;
                    chip->pc += 2;
                break;
                default:
                    printf("OP code %04X is not supported...", chip->opcode);
                break;
            }
        break;
        default:
            printf("OP code %04X is not supported", chip->opcode);
        break;
    }

}