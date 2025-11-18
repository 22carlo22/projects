library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity muxforSRAMwen is
    Port ( s : in  STD_LOGIC_VECTOR (1 downto 0);
           i1 : in  STD_LOGIC;
			  i2 : in  STD_LOGIC;
           o : out  STD_LOGIC);
end muxforSRAMwen;

architecture Behavioral of muxforSRAMwen is

begin
	process(s, i1, i2)
	begin
		if(s = "00") then
			o <= '0';
		elsif(s = "01") then
			o <= '1';
		elsif(s = "10") then
			o <= not(i1 or i2);
		end if;
	end process;


end Behavioral;

