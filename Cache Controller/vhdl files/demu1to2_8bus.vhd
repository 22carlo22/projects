library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity demu1to2_8bus is
    Port ( i : in  STD_LOGIC_VECTOR (7 downto 0);
           o0 : out  STD_LOGIC_VECTOR (7 downto 0);
           o1 : out  STD_LOGIC_VECTOR (7 downto 0);
           s : in  STD_LOGIC);
end demu1to2_8bus;

architecture Behavioral of demu1to2_8bus is

begin
	process(s, i)
	begin
		if(s = '1') then
			o1 <= i;
		else
			o0 <= i;
		end if;
	end process;
	

end Behavioral;

