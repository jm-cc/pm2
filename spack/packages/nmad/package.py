from spack import *
import platform

class Nmad(AutotoolsPackage):
    """NewMadeleine communication library"""
    homepage = "http://pm2.gforge.inria.fr/newmadeleine/"

    version('trunk', svn='https://scm.gforge.inria.fr/anonscm/svn/pm2/trunk/nmad')

    resource(
        name='building-tools',
        svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/building-tools',
        destination='.'
    )
    
    variant('debug', default=False, description='Build in debug mode')
    variant('optimize', default=True, description='Build in optimized mode')
    variant('mpi', default=True, description='Build MadMPI')
    variant('pioman', default=True, description='Build with pioman')
    variant('padicotm', default=True, description='Build with PadicoTM')

    depends_on('tbx')
    depends_on('puk')
    depends_on('hwloc')
    depends_on('pioman', when='+pioman')
    depends_on('padicotm', when='+padicotm')
    depends_on('pkgconfig')
    depends_on('autoconf', type='build')

    provides('mpi', when='+mpi')
    provides('madmpi', when='+mpi')

    build_directory = 'build'
    
    def configure_args(self):
        spec = self.spec

        config_args = [
            '--without-pukabi',
            '--disable-sampling',
            '--with-padicotm' if '+padicotm' in spec else '--without-padicotm',
            '--with-pioman' if '+pioman' in spec else '--without-pioman',
            ]
        
        config_args.extend([
            "--%s-debug"         % ('enable' if '+debug'    in spec else 'disable'),
            "--%s-optimize"      % ('enable' if '+optimize' in spec else 'disable'),
        ])

        return config_args
    
    def autoreconf(self, spec, prefix):
        autogen = Executable("./autogen.sh")
        autogen()
        
    def setup_dependent_environment(self, spack_env, run_env, dependent_spec):
        spack_env.set('MPICC',  join_path(self.prefix.bin, 'mpicc.madmpi'))
        spack_env.set('MPICXX', join_path(self.prefix.bin, 'mpicxx.madmpi'))
        spack_env.set('MPIF77', join_path(self.prefix.bin, 'mpif77.madmpi'))
        spack_env.set('MPIF90', join_path(self.prefix.bin, 'mpif90.madmpi'))
