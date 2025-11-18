library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use ieee.numeric_std.all;

entity sram is
    Port ( clk : in  STD_LOGIC;
           wr : in  STD_LOGIC;
           din : in  STD_LOGIC_VECTOR (7 downto 0);
           dout : out  STD_LOGIC_VECTOR (7 downto 0);
           add : in  STD_LOGIC_VECTOR (7 downto 0));
end sram;

architecture Behavioral of sram is
	type memory is array (0 to 255) of std_logic_vector(7 downto 0);
	signal my_memory : memory := (others => (others => '0'));
begin
	process(clk)
	begin
		if(clk'Event and clk = '1') then
			if(wr = '1') then
				my_memory(to_integer(unsigned(add))) <= din;
			else
				dout <= my_memory(to_integer(unsigned(add)));
			end if;
		end if;
	end process;


end Behavioral;
