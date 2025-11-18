library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use ieee.numeric_std.all;


entity tagvd is
    Port ( index : in  STD_LOGIC_VECTOR (2 downto 0);
           tag_in : in  STD_LOGIC_VECTOR (7 downto 0);
           tag_wr : in  STD_LOGIC;
           v_in : in  STD_LOGIC;
           d_in : in  STD_LOGIC;
           v_wr : in  STD_LOGIC;
           d_wr : in  STD_LOGIC;
           tag_out : out  STD_LOGIC_VECTOR (7 downto 0);
           hit : out  STD_LOGIC;
			  valid : out STD_LOGIC;
			  dirty : out STD_LOGIC
			);
end tagvd;

architecture Behavioral of tagvd is
	type memory is array (0 to 7) of std_logic_vector(7 downto 0);
	type mybit is array (0 to 7) of std_logic;
	signal tag_reg : memory := (others => (others => '0'));
	signal v_bit : mybit := (others => '0');
	signal d_bit : mybit := (others => '0');
begin
	
	process(index, tag_in, tag_wr)
	begin
		
		if(tag_wr = '1' or tag_in = tag_reg(to_integer(unsigned(index)))) then
			hit <= '1';
		else
			hit <= '0';
		end if;
		
		if(tag_wr = '1') then 
			tag_reg(to_integer(unsigned(index))) <= tag_in;
		end if;
		
	end process;
	
	process(index, v_wr, v_in)
	begin
		if(v_wr = '1') then
			v_bit(to_integer(unsigned(index))) <= v_in;
		end if;
	end process;
	
	process(index, d_wr, d_in)
	begin
		if(d_wr = '1') then
			d_bit(to_integer(unsigned(index))) <= d_in;
		end if;
		
	end process;
	
	tag_out <= tag_reg(to_integer(unsigned(index)));
	dirty <= d_bit(to_integer(unsigned(index)));
	valid <= v_bit(to_integer(unsigned(index)));

end Behavioral;
